#include "definitions.hpp"
#include "serdes/serdes/parameter_sink.hpp"
#include "serdes/utils/file_monitor.hpp"
#include "util.hpp"

#include <unistd.h>

using namespace xvorin::serdes;

// 后端一致性: 同一数据四种格式互转值一致; 非法输入抛解析异常
// 文件能力: sink load/save/reload + 文件监控自动重载

static std::string fmt_serialize(std::shared_ptr<SerdesTree<TFileRoot>> ep, ParameterSerdesType t)
{
    switch (t) {
#if ENABLE_JSON
    case ParameterSerdesType::PST_JSON:
        return ep->to_json();
#endif
#if ENABLE_YAML
    case ParameterSerdesType::PST_YAML:
        return ep->to_yaml();
#endif
#if ENABLE_TOML
    case ParameterSerdesType::PST_TOML:
        return ep->to_toml();
#endif
#if ENABLE_XML
    case ParameterSerdesType::PST_XML:
        return ep->to_xml();
#endif
    default:
        return std::string();
    }
}

static void fmt_deserialize(std::shared_ptr<SerdesTree<TFileRoot>> ep, ParameterSerdesType t, const std::string& s)
{
    switch (t) {
#if ENABLE_JSON
    case ParameterSerdesType::PST_JSON:
        return ep->from_json(s);
#endif
#if ENABLE_YAML
    case ParameterSerdesType::PST_YAML:
        return ep->from_yaml(s);
#endif
#if ENABLE_TOML
    case ParameterSerdesType::PST_TOML:
        return ep->from_toml(s);
#endif
#if ENABLE_XML
    case ParameterSerdesType::PST_XML:
        return ep->from_xml(s);
#endif
    default:
        return;
    }
}

static void test_cross_format_consistency()
{
    auto ep = create<TFileRoot>("r");
    ep->set("r.i", "42");
    ep->set("r.s", "cross");
    ep->create("r.v.2");
    ep->set("r.v.2", "3");

    // 四格式输出 -> 各自读回, 值必须一致
    for (const auto& t : { ParameterSerdesType::PST_JSON, ParameterSerdesType::PST_YAML,
             ParameterSerdesType::PST_TOML, ParameterSerdesType::PST_XML }) {
        const std::string out = fmt_serialize(ep, t);
        CHECK(!out.empty());
        auto ep2 = create<TFileRoot>("r2");
        fmt_deserialize(ep2, t, out);
        CHECK(ep2->get<int>("r2.i") == 42);
        CHECK(ep2->get<std::string>("r2.s") == "cross");
        CHECK(ep2->get<int>("r2.v.2") == 3);
    }
}

static void test_parse_errors()
{
    auto ep = create<TFileRoot>("r");
#if ENABLE_JSON
    CHECK_THROW(ep->from_json("{invalid"), ParseJsonException);
#endif
#if ENABLE_YAML
    CHECK_THROW(ep->from_yaml(std::string("a:\n\tb: 1\n  c: 2\n")), ParseYamlException);
#endif
#if ENABLE_TOML
    CHECK_THROW(ep->from_toml("key = "), ParseTomlException);
#endif
#if ENABLE_XML
    CHECK_THROW(ep->from_xml("<root><unclosed>"), ParseXmlException);
#endif
}

static void test_sink_file()
{
    const std::string dir = testutil::make_dir("unit_sink");
    const std::string sink = dir + "/cfg.toml";

    // 文件不存在 -> 加载失败后自动用默认值保存
    auto ep = create<TFileRoot>("r");
    ep->set_sink_file(sink, ParameterSerdesType::PST_TOML);
    CHECK(ep->is_monitoring() == false);
    CHECK(testutil::read_file(sink).find("i = 1") != std::string::npos);

    // 修改后 save 落盘, 再建新树 load 读回
    ep->set("r.i", "88");
    ep->set("r.s", "saved");
    ep->save();
    auto ep2 = create<TFileRoot>("r2");
    ep2->set_sink_file(sink, ParameterSerdesType::PST_TOML);
    CHECK(ep2->get<int>("r2.i") == 88);
    CHECK(ep2->get<std::string>("r2.s") == "saved");

    // 手动 reload: 外部改文件后 load() 生效
    testutil::write_file(sink, "i = 66\ns = \"external\"\nv = [1]\n");
    ep2->reload();
    CHECK(ep2->get<int>("r2.i") == 66);
    CHECK(ep2->get<std::string>("r2.s") == "external");

    // 未设置载体: load/save/set_sink(nullptr) 均抛 SinkfileNoSpecified
    auto ep3 = create<TFileRoot>("r3");
    CHECK_THROW({ ep3->load(); }, SinkfileNoSpecified);
    CHECK_THROW({ ep3->save(); }, SinkfileNoSpecified);
    CHECK_THROW({ ep3->set_sink(std::shared_ptr<ParameterSink>()); }, SinkfileNoSpecified);
    ep3->reload(); // 未设置载体时 reload 只打印诊断, 不崩溃(sink_id() 空值保护)
}

static void test_file_monitor()
{
    const std::string dir = testutil::make_dir("unit_monitor");
    const std::string sink = dir + "/cfg.json";

    // 开启文件监控: 外部修改文件后值自动更新(inotify 异步, 允许一定延迟)
    auto ep = create<TFileRoot>("r");
    ep->set_sink_file(sink, ParameterSerdesType::PST_JSON, true);
    CHECK(ep->is_monitoring());
    testutil::write_file(sink, R"({"i":555,"s":"mon","v":[1,2]})");
    // 等 inotify 事件 + reload
    for (int i = 0; i < 50; i++) {
        usleep(100000);
        if (ep->get<int>("r.i") == 555) {
            break;
        }
    }
    CHECK(ep->get<int>("r.i") == 555);
    CHECK(ep->get<std::string>("r.s") == "mon");
}

// 共享监听: 单线程多文件订阅、多订阅者、取消订阅、回调异常隔离
static void test_file_monitor_shared()
{
    const std::string dir = testutil::make_dir("unit_monitor_center");
    const std::string f1 = dir + "/a.json";
    const std::string f2 = dir + "/b.json";
    testutil::write_file(f1, "1");
    testutil::write_file(f2, "2");

    auto& center = xvorin::serdes::FileMonitor::instance();
    std::atomic<int> c1 { 0 }, c2 { 0 }, c3 { 0 };

    const int h1 = center.subscribe(f1, [&c1] { c1.fetch_add(1); });
    const int h2 = center.subscribe(f2, [&c2] { c2.fetch_add(1); });
    const int h3 = center.subscribe(f1, [&c3] { c3.fetch_add(1); }); // 同一路径第二个订阅者
    CHECK(h1 > 0 && h2 > 0 && h3 > 0);

    // 改文件1: 两个订阅者都触发; 改文件2: 一个触发
    testutil::write_file(f1, "11");
    for (int i = 0; i < 50 && (c1.load() == 0 || c3.load() == 0); i++) {
        usleep(100000);
    }
    CHECK(c1.load() > 0 && c3.load() > 0);

    testutil::write_file(f2, "22");
    for (int i = 0; i < 50 && c2.load() == 0; i++) {
        usleep(100000);
    }
    CHECK(c2.load() > 0);

    // 回调抛异常不影响其他订阅者
    center.subscribe(f1, [] { throw std::runtime_error("boom"); });
    const int before = c1.load();
    testutil::write_file(f1, "111");
    for (int i = 0; i < 50 && c1.load() <= before; i++) {
        usleep(100000);
    }
    CHECK(c1.load() > before); // 异常订阅者之后, 正常订阅者仍收到

    // 取消订阅后不再触发
    center.unsubscribe(h1);
    center.unsubscribe(h3);
    const int after = c1.load();
    testutil::write_file(f1, "1111");
    usleep(300000);
    CHECK(c1.load() == after);
    CHECK(c2.load() > 0); // 其余订阅不受影响

    center.unsubscribe(h2);
    center.stop(); // 收尾: 停线程(单例析构也会做)
}

// 监听句柄: 创建即监听 / stop / start / set_path / 析构自动取消
static void test_monitor_handle()
{
    const std::string dir = testutil::make_dir("unit_monitor_handle");
    const std::string f1 = dir + "/h1.json";
    const std::string f2 = dir + "/h2.json";
    const std::string f3 = dir + "/h3.json";
    testutil::write_file(f1, "1");
    testutil::write_file(f2, "2");
    testutil::write_file(f3, "3");

    auto& center = xvorin::serdes::FileMonitor::instance();
    std::atomic<int> cnt { 0 };

    // 创建即监听, 携带实体/参数/文件属性
    auto h = center.monitor("app", "app.cam", f1, [&cnt] { cnt.fetch_add(1); });
    CHECK(h != nullptr && h->monitoring());
    CHECK(h->entity() == "app" && h->index() == "app.cam" && h->path() == f1);

    testutil::write_file(f1, "11");
    for (int i = 0; i < 50 && cnt.load() == 0; i++) {
        usleep(100000);
    }
    CHECK(cnt.load() > 0);

    // stop: 不再触发
    h->stop();
    CHECK(!h->monitoring());
    const int after_stop = cnt.load();
    testutil::write_file(f1, "111");
    usleep(300000);
    CHECK(cnt.load() == after_stop);

    // start: 恢复
    h->start();
    CHECK(h->monitoring());
    const int before_start = cnt.load();
    testutil::write_file(f1, "1111");
    for (int i = 0; i < 50 && cnt.load() <= before_start; i++) {
        usleep(100000);
    }
    CHECK(cnt.load() > before_start);

    // set_path: 切换监听文件
    h->set_path(f2);
    CHECK(h->path() == f2 && h->monitoring());
    const int before_path = cnt.load();
    testutil::write_file(f2, "22");
    for (int i = 0; i < 50 && cnt.load() <= before_path; i++) {
        usleep(100000);
    }
    CHECK(cnt.load() > before_path); // 新文件触发

    // 析构自动取消订阅(独占 f3, 无其他订阅者)
    {
        auto h2 = center.monitor("t2", "x", f3, [&cnt] { cnt.fetch_add(1); });
        CHECK(h2->monitoring());
    }
    const int after_dtor = cnt.load();
    testutil::write_file(f3, "33");
    usleep(300000);
    CHECK(cnt.load() == after_dtor);

    // 收尾
    h->stop();
    center.stop();
}

// ParameterSink 抽象与 FileSink 实现: 读/原子写/扩展名推断/监听
static void test_parameter_sink()
{
    using namespace xvorin::serdes;
    const std::string dir = testutil::make_dir("unit_parameter_sink");
    const std::string f = dir + "/sink.json";

    FileSink sink(f);
    CHECK(sink.id() == f);
    CHECK(sink.format() == ParameterSerdesType::PST_JSON);

    // 原子写 + 读回
    sink.write(R"({"a":1})");
    CHECK_STR_CONTAINS(testutil::read_file(f), "\"a\":1"); // 写的是原文, 无格式化
    CHECK_STR_CONTAINS(sink.read(), "\"a\"");

    // 扩展名推断
    CHECK(FileSink(dir + "/x.yaml").format() == ParameterSerdesType::PST_YAML);
    CHECK(FileSink(dir + "/x.yml").format() == ParameterSerdesType::PST_YAML);
    CHECK(FileSink(dir + "/x.toml").format() == ParameterSerdesType::PST_TOML);
    CHECK(FileSink(dir + "/x.xml").format() == ParameterSerdesType::PST_XML);
    CHECK(FileSink(dir + "/x.unknown").format() == ParameterSerdesType::PST_STRU);
    CHECK(FileSink(dir + "/noext").format() == ParameterSerdesType::PST_STRU);

    // 读侧 ${VAR} 展开
    setenv("SERDES_TEST_SINK", dir.c_str(), 1);
    testutil::write_file(dir + "/e.json", R"({"b":2})");
    FileSink envsink("${SERDES_TEST_SINK}/e.json");
    CHECK_STR_CONTAINS(envsink.read(), "\"b\":2"); // 读的是文件原文

    // 监听: 变更触发(经共享监听中心)
    std::atomic<int> cnt { 0 };
    auto h = sink.watch("test", "sink", [&cnt] { cnt.fetch_add(1); });
    CHECK(h != nullptr && h->monitoring());
    testutil::write_file(f, R"({"a":2})");
    for (int i = 0; i < 50 && cnt.load() == 0; i++) {
        usleep(100000);
    }
    CHECK(cnt.load() > 0);
    h->stop();
}

int main()
{
    test_cross_format_consistency();
    test_parse_errors();
    test_sink_file();
    test_file_monitor();
    test_file_monitor_shared();
    test_monitor_handle();
    test_parameter_sink();
    std::cout << "[OK] backends" << std::endl;
    return 0;
}
