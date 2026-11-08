#pragma once

#include "serdes/serdes/format_type.hpp"
#include "serdes/types/parameter_tree.hpp"

#if ENABLE_JSON
#include "serdes/serdes/json/json_api.hpp"
#endif
#if ENABLE_YAML
#include "serdes/serdes/yaml/yaml_api.hpp"
#endif
#if ENABLE_TOML
#include "serdes/serdes/toml/toml_api.hpp"
#endif
#if ENABLE_XML
#include "serdes/serdes/xml/xml_api.hpp"
#endif
#if ENABLE_PROTOBUF
#include "serdes/serdes/protobuf/protobuf_api.hpp"
#endif

#include "serdes/serdes/parameter_sink.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace xvorin::serdes {

template <typename T>
class CommandLineAPI {
public:
    explicit CommandLineAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    /// @brief 命令行参数解析
    /// @param argc 参数个数
    /// @param argv 参数列表
    /// @param ignore_foreign 为 true 时忽略不属于本树的 index(其根段与树 root 不一致),
    ///        用于多组件共享同一命令行, 各组件只认领以自身名字为根的参数
    void parse_command_line(int argc, char** argv, bool ignore_foreign = false);

    /// @brief 设置参数持久化的配置文件
    /// @param sink 配置文件名称
    /// @param sink_type 配置文件格式
    /// @param monitor 是否开启文件监控, 若有改动则自动重新加载配置
    void set_sink_file(
        const std::string& sink, ParameterSerdesType sink_type = default_sink_type(), bool enable_file_monitor = false);

    /// @brief 注入数据载体(内存/网络等后端), 参数持久化走该 ParameterSink; 与 set_sink_file 同语义
    /// @param sink 数据载体(须可读/写内容文本); 内容格式取 sink->format(), 无法确定时回退默认格式
    /// @param monitor 是否开启变更监控(依赖载体 watch 支持)
    void set_sink(std::shared_ptr<ParameterSink> sink, bool enable_file_monitor = false);

    /// @brief 是否已经开启文件监控
    /// @return
    bool is_monitoring();
    /// @brief 开启/关闭所有 subtree 子文件的变更监控(文件被外部修改后自动重载子树)
    void enable_subtree_monitor(bool enable = true);
    /// @brief 细粒度控制: 仅开启/关闭指定 subtree(index, 形如 "root.sub") 的变更监控
    void enable_subtree_monitor(const std::string& index, bool enable = true);

    /// @brief 读取设置的配置文件格式
    /// @return
    ParameterSerdesType sink_type();

    /// @brief 加载配置文件中的内容
    void load();

    /// @brief 保存参数到配置文件
    void save();

    /// @brief 重新加载配置文件
    void reload();

private:
    virtual std::string serialize() = 0;
    virtual void deserialize(const std::string&) = 0;

    bool has_cmd(std::vector<std::pair<std::string, std::string>> cmds, const std::string& target, std::string* param = nullptr);
    /// @brief 丢弃 index 根段与树 root 不一致的命令(多组件共享命令行时, 各组件只认领自身参数)
    void drop_foreign_cmds(std::vector<std::pair<std::string, std::string>>& cmds);

    /// @brief 监控回调: 经树入口重载指定 subtree 并同步 value_
    void reload_monitored_subtree(const std::string& index);

    /// @brief 设置载体的公共实现; set_sink/set_sink_file 共用
    void do_set_sink(std::shared_ptr<ParameterSink> sink, bool enable_file_monitor);
    /// @brief 载体标识(路径/URL), 供诊断输出; 未设置载体时为空
    const std::string& sink_id() const
    {
        static const std::string kEmpty;
        return sink_ ? sink_->id() : kEmpty;
    }

private:
    ParameterTree<T>& tree_;
    std::string sink_content_; // 最近一次加载内容(防监控自激)
    std::recursive_mutex lock_; // 实例级粗粒度锁: public 方法整段持有;
    // 允许的同线程嵌套: parse_command_line->save; do_set_sink->load/save;
    // reload->load/save; save/load->serialize/deserialize->sink_type
    std::shared_ptr<ParameterSink> sink_; // 数据载体(默认 FileSink, 可注入)
    std::shared_ptr<Monitor> monitor_; // 主配置变更监控(经载体 watch -> 共享中心)
    /// 子树监控订阅表(index -> 句柄); 随对象析构自动取消订阅
    std::unordered_map<std::string, std::shared_ptr<Monitor>> sub_monitors_;
};

template <typename T>
void CommandLineAPI<T>::parse_command_line(int argc, char** argv, bool ignore_foreign)
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    static const auto help_info = R"(
        [ --<index>[=<value>] ] 临时修改参数值;只按首个'='切开,值内可再含'=': --vf=format=yuv420p,x=1
        [ --create   <index>  ] 临时添加参数
        [ --remove   <index>  ] 临时删除参数
        [ --save ]              将--<index>/--create/--remove等临时修改持久化到配置文件
        [ --show     [index]  ] 展示指定index的参数结构;index选填,不填时展示整体参数结构
        [ --help ]              展示此帮助信息
    )";

    // clang-format off
    // 注意: 不可加 static —— 下面的 lambda 以 [&] 捕获 this/argv,
    // 静态化会把首次调用的 this/argv 固化下来, 导致多实例(多组件)时 show/help 作用到错误的树。
    std::map<std::string, std::function<void(const std::string&)>> actions = {
        { "help",   [&](const std::string& s) { std::cout << "Usage: " << argv[0] << help_info << std::endl; exit(0); } },
        { "show",   [&](const std::string& s) { std::cout << "From " << sink_id() << "\n" << tree_.debug_string(s) << std::endl; exit(0);} },
        { "create", [&](const std::string& s) { tree_.create(s); } },
        { "remove", [&](const std::string& s) { tree_.remove(s); } },
        { "save",   nullptr }
    };
    // clang-format on

    auto trim = [](const std::string& s) {
        size_t b = 0, e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) {
            ++b;
        }
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
            --e;
        }
        return s.substr(b, e - b);
    };

    std::vector<std::pair<std::string, std::string>> cmds;
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        const std::string arg = trim(argv[i]);
        // 选项只按【首个】'=' 切开，右侧整体作为值，避免 --vf=format=yuv420p 被切碎。
        // 非选项（值 argv）不再按 '=' 切，所以 --k format=yuv 也能保住值里的 '='。
        if (arg.size() >= 2 && arg.compare(0, 2, "--") == 0) {
            const size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                const std::string k = trim(arg.substr(0, eq));
                const std::string v = trim(arg.substr(eq + 1));
                if (!k.empty()) {
                    args.push_back(k);
                }
                if (!v.empty()) {
                    args.push_back(v);
                }
                continue;
            }
        }
        if (arg.empty() || arg == "=") {
            continue; // --k = v 拆成三个 argv 时丢掉单独的 '='
        }
        args.push_back(arg[0] == '=' ? trim(arg.substr(1)) : arg); // --k =v
    }

    for (size_t i = 1; i < args.size(); i++) {
        if (0 != args[i].find("--")) {
            continue;
        }

        const std::string cmd = args[i].substr(2);
        if (actions.find(cmd) != actions.end()) {
            cmds.push_back({ cmd, (i + 1 < args.size() && 0 != args[i + 1].find("--")) ? args[++i] : "" });
            continue;
        }

        if (i + 1 >= args.size()) {
            break;
        }

        cmds.push_back({ cmd, args[++i] });
    }

    // 忽略不属于本树的 index: 其根段(首个 '.' 之前)与树 root 不一致。
    // 多组件共享同一命令行时, 各组件只认领以自身名字为根的参数; help/save 无 index 恒保留,
    // 省略 index 的 --show 也保留(展示本树整体结构)。
    if (ignore_foreign) {
        drop_foreign_cmds(cmds);
    }

    if (has_cmd(cmds, "help")) {
        actions["help"](""); // exit
    }

    // create or remove
    for (const auto& cmd : cmds) {
        if (cmd.first == "create" || cmd.first == "remove") {
            actions[cmd.first](cmd.second);
        }
    }

    // modify
    for (const auto& cmd : cmds) {
        if (actions.find(cmd.first) == actions.end()) {
            tree_.set(cmd.first, cmd.second);
        }
    }

    tree_.commit_model_changes();

    // save
    if (has_cmd(cmds, "save")) {
        save();
    }

    std::string show_index;
    if (has_cmd(cmds, "show", &show_index)) {
        actions["show"](show_index); // exit
    }
}

template <typename T>
void CommandLineAPI<T>::set_sink_file(const std::string& sink, ParameterSerdesType sink_type, bool enable_file_monitor)
{
    // 显式 type 语义保留(旧 API): 经 FileSink 的格式覆盖生效, sink_type() 由载体 format() 提供
    do_set_sink(std::make_shared<FileSink>(sink, sink_type), enable_file_monitor);
}

template <typename T>
void CommandLineAPI<T>::set_sink(std::shared_ptr<ParameterSink> sink, bool enable_file_monitor)
{
    do_set_sink(std::move(sink), enable_file_monitor);
}

template <typename T>
void CommandLineAPI<T>::do_set_sink(std::shared_ptr<ParameterSink> sink, bool enable_file_monitor)
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    sink_ = std::move(sink);
    sink_content_.clear(); // 换载体后旧内容基线失效, 否则新载体内容恰好相同会被防自激跳过
    if (!sink_) {
        throw SinkfileNoSpecified(" for set_sink");
    }

    try {
        load();
    } catch (const SinkfileMissing& e) {
        // 主载体不存在/无内容 → 视为首次配置, 写默认值;
        // 子文件(含嵌套子树引用的文件)缺失/解析失败会以 BadFile/ParseXxx 上抛, 不覆盖主配置
        std::cerr << "load " << sink_id() << " failure!" << e.what() << std::endl;
        save(); // 载体无内容/不存在, 将默认值序列化写入
        std::cout << "save " << sink_id() << " with current value!" << std::endl;
    }

    // 使能参数变更通知
    tree_.enable_inform(true);

    // 监控载体变更(依赖载体 watch 支持, 如 FileSink -> 共享文件监听)
    monitor_ = enable_file_monitor ? sink_->watch("serdes", "sink", std::bind(&CommandLineAPI<T>::reload, this)) : nullptr;
}

template <typename T>
bool CommandLineAPI<T>::is_monitoring()
{
    std::lock_guard<std::recursive_mutex> guard(lock_);
    return nullptr != monitor_;
}

template <typename T>
ParameterSerdesType CommandLineAPI<T>::sink_type()
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    // 生效格式是载体属性(sink->format(), FileSink 可含显式覆盖); 无法确定时回退默认格式
    if (!sink_) {
        return default_sink_type();
    }
    const auto fmt = sink_->format();
    return (fmt == ParameterSerdesType::PST_STRU) ? default_sink_type() : fmt;
}

template <typename T>
void CommandLineAPI<T>::load()
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    if (!sink_) {
        throw SinkfileNoSpecified(" for load");
    }

    std::string content;
    try {
        content = sink_->read(); // FileSink 读缺失/${VAR}缺失抛 BadFile/EnvarNotExist
    } catch (const BadFile&) {
        throw SinkfileMissing(sink_->id()); // 主载体缺失: 与子文件缺失区分, 供 do_set_sink 决定是否写默认值
    }

    if (content == sink_content_) {
        return; // 内容未变(含自身写入触发的回调), 静默返回
    }

    std::cout << "reload " << sink_id() << std::endl; // 仅在内容真正变化时打印

    deserialize(content);
    sink_content_.swap(content);
}

template <typename T>
void CommandLineAPI<T>::save()
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    if (!sink_) {
        throw SinkfileNoSpecified(" for save");
    }

    // 序列化闸门: 覆盖"置标志 + 序列化 + 复位"全程, 其他线程的 to_xxx/from_xxx 在此窗口被挡住,
    // 不会看到/改写 subtree_sink; 块结束后释放闸门, sink_->write 不再阻塞其他线程的序列化
    std::string content;
    {
        auto gate = tree_.lock();
        auto profile = tree_.parameter(tree_.root())->profile();
        profile->subtree_sink = true;
        try {
            content = serialize();
        } catch (...) {
            profile->subtree_sink = false;
            throw;
        }
        profile->subtree_sink = false;
    }

    sink_content_.swap(content);
    sink_->write(sink_content_); // FileSink 实现为原子写(临时文件+rename)
}

template <typename T>
void CommandLineAPI<T>::enable_subtree_monitor(const std::string& index, bool enable)
{
    std::lock_guard<std::recursive_mutex> guard(lock_);

    if (!enable) {
        auto iter = sub_monitors_.find(index);
        if (iter != sub_monitors_.end()) {
            iter->second->stop();
            sub_monitors_.erase(iter);
        }
        return;
    }

    // 树锁内读取 path/注入载体, 避免与其他线程的 deserialize 并发读写节点
    std::string path;
    std::shared_ptr<ParameterSink> injected;
    const auto visited = tree_.visit_subtree(index, [&path, &injected](Subtree& sub) {
        path = sub.path;
        injected = sub.sink;
    });
    if (!visited || path.empty()) {
        return;
    }

    // 重复开启: 旧 handle 被替换析构(自动 stop); 监听经节点的载体 -> 共享文件监听
    auto target = injected ? injected : std::make_shared<FileSink>(path); // 未注入则用默认文件载体
    auto h = target->watch("serdes", index, [this, index] { reload_monitored_subtree(index); });
    if (h) {
        sub_monitors_[index] = std::move(h);
    }
}

template <typename T>
void CommandLineAPI<T>::enable_subtree_monitor(bool enable)
{
    // 树锁内取快照, 再逐个操作, 避免遍历期间与其他线程的 create/remove 竞争
    for (const auto& index : tree_.subtree_indices()) {
        enable_subtree_monitor(index, enable);
    }
}

template <typename T>
void CommandLineAPI<T>::reload_monitored_subtree(const std::string& index)
{
    std::lock_guard<std::recursive_mutex> guard(lock_);
    tree_.reload_subtree(index, sink_type());
}

template <typename T>
void CommandLineAPI<T>::reload()
{
    std::lock_guard<std::recursive_mutex> guard(lock_);
    try {
        load();
    } catch (ViolationException& e) {
        save();
        std::cerr << "sink file " << sink_id() << " reload invalid parameter, write back to sink!" << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "sink file " << sink_id() << " changed, load failure!" << e.what() << std::endl;
    }
}

template <typename T>
bool CommandLineAPI<T>::has_cmd(std::vector<std::pair<std::string, std::string>> cmds, const std::string& target, std::string* param)
{
    for (const auto& cmd : cmds) {
        if (cmd.first == target) {
            if (param) {
                *param = cmd.second;
            }
            return true;
        }
    }
    return false;
}

template <typename T>
void CommandLineAPI<T>::drop_foreign_cmds(std::vector<std::pair<std::string, std::string>>& cmds)
{
    const std::string root = tree_.root();
    auto is_foreign = [&root](const std::string& index) {
        if (index.empty()) {
            return false;
        }
        const size_t dot = index.find('.');
        const std::string head = (dot == std::string::npos) ? index : index.substr(0, dot);
        return head != root;
    };
    // help/save/show/create/remove 的 index 在 second; 其余(参数修改)的 index 在 first
    static const std::set<std::string> kNamedCmds { "help", "save", "show", "create", "remove" };
    auto is_foreign_cmd = [&is_foreign](const std::pair<std::string, std::string>& cmd) {
        const bool named_cmd = kNamedCmds.find(cmd.first) != kNamedCmds.end();
        return is_foreign(named_cmd ? cmd.second : cmd.first);
    };

    cmds.erase(std::remove_if(cmds.begin(), cmds.end(), is_foreign_cmd), cmds.end());
}

template <typename T, typename... APIS>
class ExtendedParameterTree : public ParameterTree<T>, public APIS... {
public:
    /// @brief 每次调用都创建一个新的实例
    /// @param root 实例名称
    /// @return 返回新创建的实例
    static std::shared_ptr<ExtendedParameterTree> create_instance(const std::string& root = "root")
    {
        return std::shared_ptr<ExtendedParameterTree>(new ExtendedParameterTree(root));
    }

    /// @brief 创建或返回已建实例(以实例名称作为索引)
    /// @param root 实例名称
    /// @return 返回新建或已建实例
    static std::shared_ptr<ExtendedParameterTree> get_or_create_instance(const std::string& root)
    {
        static std::mutex lock;
        static std::unordered_map<std::string, std::shared_ptr<ExtendedParameterTree>> instances;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto iter = instances.find(root);
            if (iter != instances.end()) {
                return iter->second;
            }
            return instances[root] = create_instance(root);
        }
    }

private:
    ExtendedParameterTree(const std::string& root)
        : ParameterTree<T>(root)
        , APIS(static_cast<ParameterTree<T>&>(*this))...
    {
    }

private:
    std::string serialize()
    {
        std::string out;

#if ENABLE_JSON
        if (this->sink_type() == ParameterSerdesType::PST_JSON) {
            out = this->to_json();
        }
#endif

#if ENABLE_YAML
        if (this->sink_type() == ParameterSerdesType::PST_YAML) {
            out = this->to_yaml();
        }
#endif

#if ENABLE_TOML
        if (this->sink_type() == ParameterSerdesType::PST_TOML) {
            out = this->to_toml();
        }
#endif

#if ENABLE_XML
        if (this->sink_type() == ParameterSerdesType::PST_XML) {
            out = this->to_xml();
        }
#endif

#if ENABLE_PROTOBUF
        if (this->sink_type() == ParameterSerdesType::PST_PBFMT) {
            out = this->to_pbtxt();
        }
        if (this->sink_type() == ParameterSerdesType::PST_PBBIN) {
            out = this->to_pbbin();
        }
#endif

        return out;
    }

    void deserialize(const std::string& in)
    {
#if ENABLE_YAML
        if (this->sink_type() == ParameterSerdesType::PST_YAML) {
            this->from_yaml(in);
        }
#endif

#if ENABLE_JSON
        if (this->sink_type() == ParameterSerdesType::PST_JSON) {
            this->from_json(in);
        }
#endif

#if ENABLE_TOML
        if (this->sink_type() == ParameterSerdesType::PST_TOML) {
            this->from_toml(in);
        }
#endif

#if ENABLE_XML
        if (this->sink_type() == ParameterSerdesType::PST_XML) {
            this->from_xml(in);
        }
#endif

#if ENABLE_PROTOBUF
        if (this->sink_type() == ParameterSerdesType::PST_PBFMT) {
            this->from_pbtxt(in);
        }
        if (this->sink_type() == ParameterSerdesType::PST_PBBIN) {
            this->from_pbbin(in);
        }
#endif
    }
};

template <typename T>
using SerdesTree = ExtendedParameterTree<T, CommandLineAPI<T>
#if ENABLE_JSON
    ,
    JsonAPI<T>
#endif
#if ENABLE_YAML
    ,
    YamlAPI<T>
#endif
#if ENABLE_TOML
    ,
    TomlAPI<T>
#endif
#if ENABLE_XML
    ,
    XmlAPI<T>
#endif
#if ENABLE_PROTOBUF
    ,
    ProtobufAPI<T>
#endif
    >;

template <typename T>
std::shared_ptr<SerdesTree<T>> create(const std::string& root = "root")
{
    return SerdesTree<T>::create_instance(root);
}

template <typename T>
std::shared_ptr<SerdesTree<T>> instance(const std::string& root)
{
    return SerdesTree<T>::get_or_create_instance(root);
}

template <typename T>
using entity = std::shared_ptr<SerdesTree<T>>;

}