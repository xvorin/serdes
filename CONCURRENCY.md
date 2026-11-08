# serdes 并发使用说明

本文说明当前实现下，多线程使用 serdes 时**可以做什么、不可以做什么、需要注意什么**。

## 1. 线程模型

- 每个参数树实例内部有一把**整树锁（数据锁）**和一把**序列化闸门**；每个 `CommandLineAPI` 实例内部有一把**整实例锁**；
  文件监听中心有一把订阅表锁；每个 `Monitor` 有一把句柄锁。
- 锁只在“实例”这一层，没有节点级/字段级锁。
- 单次 public API 调用是原子的：调用期间其他线程对该实例的访问会等待。
- 多步操作（多次 API 调用）之间仍可能被其他线程插入，需要调用方自己保证事务性。

## 2. 可以做什么

任意线程可以并发调用任意 public API，包括：

- `get / set / value / debug_string`
- `save / load / reload`
- `set_sink / set_sink_file / sink_type / is_monitoring`
- `enable_inform / enable_subtree_inline`
- `create / remove / parameter / exist / instance`
- `enable_subtree_monitor / Monitor::start / stop / set_path`
- 同一实例上的 `parse_command_line`

单次调用的内部保证：

- `get` 不会读到写了一半的结构；
- `set` 与 `save` 自动串行，`save` 写出的一定是一致的快照；
- `load` 反序列化期间 `get` 会等待，不会看到半加载状态；
- `set_sink` 切换载体期间，其他读写会等待到切换完成；
- 不同实例之间互不阻塞（各自持各自的锁）。

文件监控回调（运行在库的监控线程）可以直接调用 core API，库会用同一套锁串行化。

## 3. 不可以做什么

1. **不要依赖多次调用之间的原子性**：
   ```cpp
   // 错误: get 和 set 之间可能被其他线程插入
   tree->set("app.count", std::to_string(tree->get<int>("app.count") + 1));
   ```
   需要外部锁：
   ```cpp
   {
       std::lock_guard<std::mutex> lock(app_mutex);
       tree->set("app.count", std::to_string(tree->get<int>("app.count") + 1));
   }
   ```
   或用 `mutable_value()` 一次持锁完成。

2. **不要在同步回调里重入 API**：
   - inform 回调、validator、自定义序列化回调都在 API 调用内部、锁已持有的情况下执行；
   - 在这些回调里再调用本实例的 API 会等待同一把锁，导致死锁。

3. **不要在文件监控回调里做长耗时/阻塞操作**：
   - 回调运行在监控线程上，阻塞会拖住所有文件的监听；
   - 不要在回调里调用 `FileMonitor::stop()`（会尝试 join 当前线程）；
   - 不要在回调里销毁回调捕获的对象。

4. **不要在调用期间释放对象**：
   调用方必须持有对象（`shared_ptr` 或栈对象）直到调用返回；不要在另一个线程正在调用时释放最后一个引用。

5. **不要给同一棵树挂多个 `CommandLineAPI`**：
   一个树实例配一个 API 实例；多个 API 共享同一棵树时锁序和语义不保证。

6. **不要在持有外部锁时等待监控线程**：
   例如持有业务锁调用 `FileMonitor::stop()`，而监控回调正在等这把业务锁，会互相等待。

7. **`FileMonitor::stop()` 只做生命周期收尾**：
   不要与其他线程的 `subscribe/unsubscribe/Monitor::start/stop/set_path` 并发调用；
   它返回后所有订阅被清空，需要重新开启监控。

8. **不要直接修改 `profile()` 返回的字段**：
   `ParameterProfile` 是树内共享的可写结构，但它的字段只能由库在锁内修改
   （`enable_inform/enable_subtree_inline` 在数据锁内，`save` 在序列化闸门内设置 `subtree_sink`）。
   外部直接写会和并发序列化形成数据竞争；尤其不要置位 `subtree_sink`，
   否则普通 `to_xxx` 会误把子树内容落盘。

## 4. 文件监控回调

两类回调的行为不同：

| 回调 | 运行线程 | 能否调用 API |
|---|---|---|
| inform / validator / 自定义序列化 | 调用线程 | **不能重入**（锁已持有） |
| `watch()` / `enable_subtree_monitor` 的文件变化回调 | 监控线程 | 可以，但必须短小、不阻塞、不 stop 中心、不销毁捕获对象 |

- 文件变化回调抛出的异常会被监控线程吞掉，不会传播。
- 同一个文件可能产生多次事件，回调必须幂等。
- 库内部对配置文件的自动重载就是通过文件回调完成的，重载过程会正常获取实例锁和树锁。

## 5. 生命周期

对象被回调捕获时，销毁前先停止监听：

```cpp
tree->enable_subtree_monitor(false);   // 不再产生新的子树回调
FileMonitor::instance().stop();  // 停止监控线程并等待回调结束
tree.reset();                          // 再销毁对象
```

- `Monitor::stop()` / `enable_subtree_monitor(false)` 返回后不会再产生**新**回调；
  但如果不调用 `center.stop()`，已进入执行的回调可能仍在运行。
- 因此，销毁被回调捕获的对象前，要么确认没有在途回调，要么调用 `center.stop()`。
- `FileMonitor` 是进程级单例：任一组件 `stop()` 后，其他组件的监听也会一起失效；
  共享使用时需要协调，重新使用前要重新 `subscribe` / `enable_subtree_monitor`。

## 6. 文件与多进程

- 写文件采用**唯一临时文件 + `rename`**：临时名带 pid 与进程内序号，多个进程/工具并发写时各自写独立的临时文件，`rename` 原子替换目标。
  因此**配置文件永远不会损坏**：读者任意时刻看到的都是完整旧内容或完整新内容，并发结果是“最后写者胜”。
- 同一进程内多个线程 `save()` 同一实例会被实例锁串行化。
- 需要“读-改-写”一致性（例如两个进程都先加载再保存）时，库不提供跨进程互斥，
  请自行持独立的锁文件（如 `path + ".lock"`）覆盖整个事务；单纯防损坏不需要锁。
- 与 vim 编辑同一个文件：
  - 建议 vim 设置 `backupcopy=no`（vim 也走“写临时文件 + rename”），双方行为一致，任何时刻都可解析；
  - 如果 vim 原地写（默认 auto 在硬链接/NFS/权限受限场景可能判定为原地写），写入那一瞬间手动 `load()` 可能读到半截而解析失败，可重试一次；文件监控的自动重载会因后续 `IN_CLOSE_WRITE` 自动恢复。
- 路径中的 `${VAR}` 在读写、监听三处都会展开；环境变量缺失会抛 `EnvarNotExist`。
- `save()` 在持有实例锁期间完成序列化和写盘，其他线程的 `get/set` 会等待；
  配置保存频率较高或对延迟敏感时，注意调用时机。
- **自激抑制**：主配置 `load()` 与文件内容相同则跳过（`sink_content_` 基线）；子树 `load()` 读到与
  最近一次写回/加载相同的内容时也跳过重载。因此 `save()` 自己写回的子树文件触发的监控事件不会造成
  无谓重载，也不会用旧文件内容覆盖内存中未保存的修改。

## 7. `mutable_value()`

- 返回裸指针并转移树锁：持有 `std::unique_lock` 期间，其他线程的树操作会阻塞。
- 适合“多字段同时修改”的事务场景，单个字段优先用 `set()`。
- 拿到指针后尽快修改、尽快释放锁；不要在持有期间做文件 I/O、网络等待或调用其他 API。

## 8. 使用示例

### 8.1 直接多线程使用

```cpp
auto tree = create<MyConfig>("app");
tree->set_sink_file("/etc/app/config.json", ParameterSerdesType::PST_JSON);

// 线程 A
tree->set("app.threads", "8");

// 线程 B
const int threads = tree->get<int>("app.threads");

// 线程 C
tree->save();
```

每次调用都是原子的，不需要外部锁。

### 8.2 事务性修改

```cpp
std::mutex app_mutex;

void add_task()
{
    std::lock_guard<std::mutex> lock(app_mutex);
    const int count = tree->get<int>("app.task_count");
    tree->set("app.task_count", std::to_string(count + 1));
    tree->create("app.tasks." + std::to_string(count));
}
```

### 8.3 文件监控

```cpp
tree->enable_subtree_monitor(true);
// 文件变化 -> 监控线程 -> 自动重载(内部会加实例锁和树锁)

auto handle = sink.watch("app", "app.cam", [&] {
    const int x = tree->get<int>("app.cam.x"); // 安全, 但回调应短小
});
```

## 9. 库的保证与不保证

保证：

- 任意线程单次调用 public API 安全；
- `save/load` 与 `get/set` 自动串行，不会看到中间状态；
- `unsubscribe()` / `Monitor::stop()` 返回后，不再产生该订阅的**新**回调；
  已在执行的回调不会被等待，可能继续执行完；
- `FileMonitor::stop()` 返回后监控线程已退出，不再产生任何新回调；
- 文件写入不产生半截内容。

不保证：

- 多次调用组成的事务的原子性；
- 回调的业务顺序、去重、只触发一次；
- 同一棵树挂多个 `CommandLineAPI` 的并发语义；
- 跨进程读-改-写一致性；
- 在途回调与对象析构的自动隔离（需按第 5 节停监听）；
- 同步回调（inform/validator）里的重入。

## 10. 快速检查清单

- [ ] 多步读改写是否加了外部锁或使用了 `mutable_value()`？
- [ ] inform/validator/序列化回调里是否避免了调用 API？
- [ ] 文件监控回调是否短小、没有阻塞等待、没有 stop 中心？
- [ ] 销毁被回调捕获的对象前，是否停止了监听并（必要时）调用了 `center.stop()`？
- [ ] 是否避免了在持有业务锁时等待监控线程？
- [ ] 是否避免了在并发订阅/控制监听时调用 `FileMonitor::stop()`？
- [ ] 同一棵树是否只配了一个 `CommandLineAPI`？
- [ ] 跨进程读改写是否有独立的跨进程锁？
- [ ] 对延迟敏感的线程是否避免了频繁 `save()`？

## 11. 代码中的锁

### 11.1 锁一览

| 锁 | 类型 | 保护对象 | 谁持有 |
|---|---|---|---|
| `ParameterTree::lock_` | `std::mutex` | 参数数据：`model_` / `value_` / profile 字段 | 树的 public 方法；`mutable_value` 转移给调用方 |
| `ParameterTree::serialize_lock_` | `std::recursive_mutex` | 序列化互斥 + `profile->subtree_sink` 标志窗口 | `serialize/deserialize/reload_subtree`；`save` 经 `tree_.lock()` 显式持有 |
| `CommandLineAPI::lock_` | `std::recursive_mutex` | 载体与监控状态：`sink_` / `sink_content_` / `monitor_` / `sub_monitors_` | `save/load/reload/set_sink*/sink_type/is_monitoring/parse_command_line/enable_subtree_monitor` 等 public 入口 |
| `FileMonitor::lock_` | `std::mutex` | 订阅表：`by_path_` / `handle_path_` / `wd_to_dir_` | `subscribe/unsubscribe` 与监控线程 `dispatch` |
| `Monitor::lock_` | `std::mutex` | 句柄状态：`handle_` / `path_` | `start/stop/set_path/monitoring/path` |
| `running_` | `std::atomic<bool>` | 监控线程退出标志 | 监控线程读，`stop()` 写 |
| `get_or_create_instance` 的 static mutex | `std::mutex` | 实例注册表 | `instance<T>(root)` 首次创建 |

无锁设施：

- `profile->subtree_sink`：普通成员，只在**闸门 + 数据锁**内读写；
- 文件原子写：唯一临时文件 + `rename`；
- `FileSink` 的 `path_` 构造后只读。

### 11.2 每把锁的作用分析

#### `ParameterTree::lock_` —— 数据锁

- 保护 `model_/value_` 以及 profile 上的 `serdes_type/violations/inform_enabled/subtree_inline`。
- `get/set/value/create/remove/debug_string/enable_*/serialize/deserialize/reload_subtree` 都在这把锁内完成对树结构的访问。
- **非递归**：同一线程二次获取会死锁。`mutable_value()` 把锁转移给调用方，因此文档约定“持锁期间不要调用任何树/API 接口”（见第 7 节）。
- 竞争点：`get/set` 与 `save/load` 的序列化段互斥；`save/load` 的**文件 I/O 不持有数据锁**，慢盘/NFS 不会阻塞 `get/set`。

#### `ParameterTree::serialize_lock_` —— 序列化闸门（递归）

- 保护两件事：序列化/反序列化的互斥；`profile->subtree_sink` 的“置位 + 遍历 + 复位”窗口。
- `save()` 通过 `tree_.lock()` 显式持闸门，设置 `subtree_sink = true` 后调用 `serialize()`；闸门是递归的，`serialize → to_xxx → tree.serialize` 的同线程重入不冲突。
- 其他线程的 `to_xxx/from_xxx` 在 `serialize/deserialize` 入口也会拿闸门，因此 **save 的序列化期间它们被挡住，看不到 `subtree_sink`**。
- `get/set` 不拿闸门，只拿数据锁；所以 save 窗口内它们仍可执行（序列化段除外）。
- `save()` 在 `sink_->write()` 之前释放闸门，慢载体 I/O 不会拖住 `to_xxx`。
- 锁序：**闸门 → 数据锁**。

#### `CommandLineAPI::lock_` —— 实例锁（递归）

- 保护 `sink_/sink_content_/monitor_/sub_monitors_`，并把 `save/load/reload/set_sink*` 包成完整调用。
- **必须递归**，嵌套链固定：
  `parse_command_line → save`；`do_set_sink → load/save`；`reload → load/save`；`save/load → serialize/deserialize → sink_type`。
- 锁序：**实例锁 → 闸门 → 数据锁**。

#### `FileMonitor::lock_` —— 订阅表锁

- 保护 `by_path_/handle_path_/wd_to_dir_`。
- `dispatch` 在锁内只做“快照回调”，释放锁后才执行回调；回调再按 `实例锁 → 闸门 → 数据锁` 进入 core，不会反向持锁。
- 锁内绝不调用回调，避免回调再订阅造成递归死锁。

#### `Monitor::lock_` —— 句柄锁

- 保护 `handle_/path_`；`start/stop/set_path/monitoring/path` 可被任意线程调用。
- `path()` 返回**拷贝**，避免调用方拿到引用后被并发 `set_path` 改写。

#### `running_` / 实例注册表锁

- `running_`：监控循环退出标志，`stop()` 置位后 `join`。
- 注册表锁：`instance<T>(root)` 并发首次创建同一 root 时互斥。

### 11.3 锁序与红线

全局锁序：

```text
CommandLineAPI::lock_ → ParameterTree::serialize_lock_ → ParameterTree::lock_
Monitor::lock_      → FileMonitor::lock_
```

红线（违反会死锁）：

- 持有 `ParameterTree::lock_`（含 `mutable_value` 转移的锁）时，不要再调用树/API 方法；
- 持任何锁时不要调用 `FileMonitor::stop()`（它会 join 监控线程，而回调可能在等同一把锁）；
- `FileMonitor::lock_` 内不执行回调；
- `serialize_lock_` 的 `lock()` 只开放给 `CommandLineAPI`（`friend`），外部不要自行持有；
- 不要直接写 `profile()` 的字段（尤其 `subtree_sink`），它只能在库内加锁修改。
