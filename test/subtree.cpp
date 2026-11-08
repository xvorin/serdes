#include "definitions.hpp"
#include "util.hpp"

#include <unistd.h>

using namespace xvorin::serdes;

// subtree: 单元素/容器、外置文件加载/内联/环境变量、路径往返、结构体同步、错误路径、各后端

static void setup_files(const std::string& dir)
{
    testutil::write_file(dir + "/sub.json", R"({"x":9,"y":"subX"})");
    testutil::write_file(dir + "/sub2.json", R"({"x":42,"y":"subY"})");
    testutil::write_file(dir + "/sub.yaml", "x: 91\ny: subY2\n");
    testutil::write_file(dir + "/sub.toml", "x = 92\ny = \"subT\"\n");
    testutil::write_file(dir + "/sub.xml", "<subcfg><x>93</x><y>subX2</y></subcfg>");
}

static void test_single_subtree_json()
{
    const std::string dir = testutil::make_dir("unit_subtree");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree/sub.json"})");

    // 子树挂入整体树, 普通参数 API 可用
    CHECK(ep->get<int>("r.sub.x") == 9);
    CHECK(ep->get<std::string>("r.sub.y") == "subX");
    CHECK_STR_CONTAINS(ep->debug_string(), "r.sub.x = 9 [int]");

    // set 穿透子树
    ep->set("r.sub.x", "10");
    CHECK(ep->get<int>("r.sub.x") == 10);

    // 结构体缓存同步
    auto v = ep->value();
    CHECK(v.sub._subtree_sink_path_ == dir + "/sub.json");
    CHECK(v.sub.x == 10 && v.sub.y == "subX");

    // 外部格式往返只携带路径; 对端 from_json 重新从文件加载(x 回到文件值 9)
    const std::string j = ep->to_json();
    CHECK_STR_CONTAINS(j, "\"sub\": \"" + dir + "/sub.json\"");
    auto ep2 = create<TSubtreeRoot>("r2");
    ep2->from_json(j);
    CHECK(ep2->get<int>("r2.sub.x") == 9);
    CHECK(ep2->value().sub._subtree_sink_path_ == dir + "/sub.json");
}

static void test_subtree_inline()
{
    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"i":2,"sub":{"x":7,"y":"inline"}})");
    CHECK(ep->get<int>("r.sub.x") == 7);
    CHECK(ep->get<std::string>("r.sub.y") == "inline");
}

static void test_subtree_env_path()
{
    const std::string dir = testutil::make_dir("unit_subtree_env");
    setup_files(dir);
    setenv("SERDES_TEST_UNIT_DIR", dir.c_str(), 1);

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"sub":"${SERDES_TEST_UNIT_DIR}/sub.json"})");
    CHECK(ep->get<int>("r.sub.x") == 9);
    CHECK(ep->value().sub._subtree_sink_path_ == "${SERDES_TEST_UNIT_DIR}/sub.json");

    // 写侧与读侧一致: 路径里的 ${VAR} 先展开, 内容落到真实子文件;
    // 若未展开, write 会因目录不存在抛 BadFile, 或创建字面量 ${...} 路径
    testutil::write_file(dir + "/main_env.json", R"({"i":2,"sub":"${SERDES_TEST_UNIT_DIR}/sub.json"})");
    auto eps = create<TSubtreeRoot>("s");
    eps->set_sink_file(dir + "/main_env.json", ParameterSerdesType::PST_JSON);
    eps->set("s.sub.x", "31");
    eps->save();
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub.json"), "\"x\": 31");
    CHECK(access("${SERDES_TEST_UNIT_DIR}", F_OK) != 0);
}

static void test_subtree_other_formats()
{
    const std::string dir = testutil::make_dir("unit_subtree_fmt");
    setup_files(dir);

    // yaml: 字符串路径 -> 读 yaml 子文件
    auto epy = create<TSubtreeRoot>("ry");
    epy->from_yaml("i: 2\nsub: /tmp/serdes_test/unit_subtree_fmt/sub.yaml\n");
    CHECK(epy->get<int>("ry.sub.x") == 91);

    // toml
    auto ept = create<TSubtreeRoot>("rt");
    ept->from_toml("i = 2\nsub = \"/tmp/serdes_test/unit_subtree_fmt/sub.toml\"\n");
    CHECK(ept->get<int>("rt.sub.x") == 92);

    // xml
    auto epx = create<TSubtreeRoot>("rx");
    epx->from_xml("<root><i>2</i><sub>/tmp/serdes_test/unit_subtree_fmt/sub.xml</sub></root>");
    CHECK(epx->get<int>("rx.sub.x") == 93);
}

static void test_subtree_in_containers()
{
    const std::string dir = testutil::make_dir("unit_subtree_cont");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"i":2,
        "subs":["/tmp/serdes_test/unit_subtree_cont/sub.json","/tmp/serdes_test/unit_subtree_cont/sub2.json"],
        "submap":{"a":"/tmp/serdes_test/unit_subtree_cont/sub.json"}})");

    CHECK(ep->get<int>("r.subs.0.x") == 9);
    CHECK(ep->get<int>("r.subs.1.x") == 42);
    CHECK(ep->get<std::string>("r.submap.a.y") == "subX");

    // set 容器内元素子树
    ep->set("r.submap.a.x", "100");
    CHECK(ep->get<int>("r.submap.a.x") == 100);

    // 结构体同步
    auto v = ep->value();
    CHECK(v.subs.size() == 2 && v.subs[1].x == 42);
    CHECK(v.submap.at("a").x == 100);

    // 往返保持路径
    CHECK_STR_CONTAINS(ep->to_json(), "/tmp/serdes_test/unit_subtree_cont/sub2.json");
}

static void test_subtree_errors()
{
    const std::string dir = testutil::make_dir("unit_subtree_err");

    // 文件缺失 -> BadFile
    auto ep = create<TSubtreeRoot>("r");
    CHECK_THROW({ ep->from_json(R"({"sub":"/tmp/serdes_test/unit_subtree_err/missing.json"})"); }, BadFile);

    // 子文件内容非法(同格式解析失败) -> ParseJsonException
    testutil::write_file(dir + "/bad.json", "{not json");
    CHECK_THROW({ ep->from_json(R"({"sub":"/tmp/serdes_test/unit_subtree_err/bad.json"})"); }, ParseJsonException);

    // 无扩展名子文件: 回退主格式(toml)解析, 内容为 json -> ParseTomlException
    // (带 .json 扩展名的子文件现在按扩展名解析, 交叉混用是合法行为)
    testutil::write_file(dir + "/sub_noext", R"({"x":9})");
    CHECK_THROW({ ep->from_toml(std::string("sub = \"/tmp/serdes_test/unit_subtree_err/sub_noext\"\n")); }, ParseTomlException);
}

// profile 开关: to_xxx 输出子树内容对象(内联)而非路径字符串
static void test_subtree_inline_output()
{
    const std::string dir = testutil::make_dir("unit_subtree_inline_out");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_inline_out/sub.json"})");

    // 默认(关): 输出路径字符串
    CHECK_STR_CONTAINS(ep->to_json(), "\"sub\": \"/tmp/serdes_test/unit_subtree_inline_out/sub.json\"");

    // 开启: 输出子树内容对象
    ep->enable_subtree_inline(true);
    const std::string j = ep->to_json();
    CHECK_STR_CONTAINS(j, "\"x\": 9");
    CHECK(j.find("\"sub\": \"/tmp/serdes_test") == std::string::npos);

    // yaml/toml/xml 同
    CHECK_STR_CONTAINS(ep->to_yaml(), "x: 9");
    CHECK_STR_CONTAINS(ep->to_toml(), "x = 9");
    CHECK_STR_CONTAINS(ep->to_xml(), "<x>9</x>");

    // 内联输出可被对端读回(输入侧本就支持内联对象), round-trip 自洽
    auto ep2 = create<TSubtreeRoot>("r2");
    ep2->from_json(j);
    CHECK(ep2->get<int>("r2.sub.x") == 9);

    // 关闭恢复路径
    ep->enable_subtree_inline(false);
    CHECK_STR_CONTAINS(ep->to_json(), "\"sub\": \"/tmp/serdes_test/unit_subtree_inline_out/sub.json\"");

    // 容器内 subtree 同样内联
    ep->enable_subtree_inline(true);
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_inline_out/sub.json","subs":["/tmp/serdes_test/unit_subtree_inline_out/sub2.json"]})");
    CHECK_STR_CONTAINS(ep->to_json(), "\"x\": 42"); // subs[0] 内容内联
}

// 交叉混用: 主/子文件格式不必一致, 子文件按扩展名推断
static void test_subtree_cross_format()
{
    const std::string dir = testutil::make_dir("unit_subtree_cross");
    setup_files(dir); // sub.json / sub2.json / sub.yaml / sub.toml / sub.xml

    // toml 主文件引用 json 子文件 -> 按 .json 解析成功
    auto ept = create<TSubtreeRoot>("rt");
    ept->from_toml(std::string("i = 2\nsub = \"" + dir + "/sub.json\"\n"));
    CHECK(ept->get<int>("rt.sub.x") == 9);

    // yaml 主文件引用 toml 子文件
    auto epy = create<TSubtreeRoot>("ry");
    epy->from_yaml(std::string("i: 2\nsub: " + dir + "/sub.toml\n"));
    CHECK(epy->get<int>("ry.sub.x") == 92);

    // json 主文件引用 xml 子文件
    auto epj = create<TSubtreeRoot>("rj");
    epj->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_cross/sub.xml"})");
    CHECK(epj->get<int>("rj.sub.x") == 93);

    // save 写回同样按扩展名: toml 主 + json 子 -> 子文件仍是 json 格式, 可被 json 主文件读回
    ept->set("rt.sub.x", "66");
    ept->set_sink_file(dir + "/cross_main.toml", ParameterSerdesType::PST_TOML);
    ept->save();
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub.json"), "\"x\": 66");
    auto round = create<TSubtreeRoot>("rr");
    round->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_cross/sub.json"})");
    CHECK(round->get<int>("rr.sub.x") == 66);
}

static void test_subtree_path_only()
{
    // set(index, path) 只改引用路径, 内容随下次加载生效
    const std::string dir = testutil::make_dir("unit_subtree_path");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"sub":"/tmp/serdes_test/unit_subtree_path/sub.json"})");
    CHECK(ep->get<int>("r.sub.x") == 9);
    ep->set("r.sub", "/tmp/serdes_test/unit_subtree_path/sub2.json");
    CHECK(ep->value().sub._subtree_sink_path_ == "/tmp/serdes_test/unit_subtree_path/sub2.json");
    // 子树内容仍是上次加载的(未重新加载)
    CHECK(ep->get<int>("r.sub.x") == 9);
    // 重新加载后生效
    ep->from_json(R"({"sub":"/tmp/serdes_test/unit_subtree_path/sub2.json"})");
    CHECK(ep->get<int>("r.sub.x") == 42);
}

static void test_save_syncs_subtree_file()
{
    const std::string dir = testutil::make_dir("unit_subtree_save");
    setup_files(dir);
    testutil::write_file(dir + "/sub3.json", R"({"x":1,"y":"s3"})");

    // 首次 set_sink_file: 主文件不存在 -> save 默认值(此时无 path, 不写子文件)
    auto ep = create<TSubtreeRoot>("r");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_save/sub.json",
        "subs":["/tmp/serdes_test/unit_subtree_save/sub2.json"],
        "submap":{"a":"/tmp/serdes_test/unit_subtree_save/sub3.json"}})");
    ep->set("r.sub.x", "77");
    ep->set("r.subs.0.x", "88");
    ep->set("r.submap.a.y", "savedY");
    ep->save();

    // 主文件仍是路径字符串
    const auto main_content = testutil::read_file(dir + "/main.json");
    CHECK_STR_CONTAINS(main_content, "\"sub\": \"/tmp/serdes_test/unit_subtree_save/sub.json\"");

    // 各子文件已同步为修改后的内容
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub.json"), "\"x\": 77");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub2.json"), "\"x\": 88");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub3.json"), "\"y\": \"savedY\"");

    // toml sink: 子文件按 toml 写回
    auto ept = create<TSubtreeRoot>("rt");
    ept->set_sink_file(dir + "/main.toml", ParameterSerdesType::PST_TOML);
    ept->from_toml(std::string("i = 2\nsub = \"/tmp/serdes_test/unit_subtree_save/sub.toml\"\n"));
    ept->set("rt.sub.x", "66");
    ept->save();
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub.toml"), "x = 66");
}

// 嵌套: 被引用的配置里再引用一份配置(main -> mid -> leaf), 加载/写回/重载/监控逐级生效
static void test_subtree_nested()
{
    const std::string dir = testutil::make_dir("unit_subtree_nested");
    testutil::write_file(dir + "/leaf.json", R"({"a":7})");
    testutil::write_file(dir + "/mid.json", R"({"m":20,"leaf":"/tmp/serdes_test/unit_subtree_nested/leaf.json"})");
    testutil::write_file(dir + "/main.json", R"({"i":1,"mid":"/tmp/serdes_test/unit_subtree_nested/mid.json"})");

    auto ep = create<TNestedRoot>("rn");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("rn.mid.m") == 20);
    CHECK(ep->get<int>("rn.mid.leaf.a") == 7);

    // 两层结构体缓存与路径同步
    auto v = ep->value();
    CHECK(v.mid._subtree_sink_path_ == dir + "/mid.json");
    CHECK(v.mid.m == 20);
    CHECK(v.mid.leaf._subtree_sink_path_ == dir + "/leaf.json");
    CHECK(v.mid.leaf.a == 7);

    // 改最深层 -> save 逐级写回; 中间层/主文件仍只存路径
    ep->set("rn.mid.leaf.a", "8");
    ep->set("rn.mid.m", "21");
    ep->save();
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/leaf.json"), "\"a\": 8");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/mid.json"), "\"m\": 21");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/mid.json"), "\"leaf\": \"" + dir + "/leaf.json\"");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/main.json"), "\"mid\": \"" + dir + "/mid.json\"");
    CHECK(ep->value().mid.leaf.a == 8);

    // 重载中间层: 其引用的下级文件跟着级联重载
    testutil::write_file(dir + "/mid.json", R"({"m":30,"leaf":"/tmp/serdes_test/unit_subtree_nested/leaf.json"})");
    testutil::write_file(dir + "/leaf.json", R"({"a":9})");
    ep->reload_subtree("rn.mid", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("rn.mid.m") == 30);
    CHECK(ep->get<int>("rn.mid.leaf.a") == 9);
    CHECK(ep->value().mid.leaf.a == 9);

    // 只重载最深层
    testutil::write_file(dir + "/leaf.json", R"({"a":10})");
    ep->reload_subtree("rn.mid.leaf", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("rn.mid.leaf.a") == 10);

    // 嵌套监控: 最深层文件变化经监听回调重载
    ep->enable_subtree_monitor("rn.mid.leaf", true);
    testutil::write_file(dir + "/leaf.json", R"({"a":11})");
    for (int i = 0; i < 50; i++) {
        usleep(100000);
        if (ep->get<int>("rn.mid.leaf.a") == 11) {
            break;
        }
    }
    CHECK(ep->get<int>("rn.mid.leaf.a") == 11);
    CHECK(ep->value().mid.leaf.a == 11);
    ep->enable_subtree_monitor(false);

    // 每层按自身扩展名解析: 主 json -> 中间 yaml -> 叶子 json
    testutil::write_file(dir + "/mid.yaml", "m: 40\nleaf: /tmp/serdes_test/unit_subtree_nested/leaf.json\n");
    testutil::write_file(dir + "/leaf.json", R"({"a":12})");
    testutil::write_file(dir + "/main2.json", R"({"i":2,"mid":"/tmp/serdes_test/unit_subtree_nested/mid.yaml"})");
    auto ep2 = create<TNestedRoot>("rn2");
    ep2->set_sink_file(dir + "/main2.json", ParameterSerdesType::PST_JSON);
    CHECK(ep2->get<int>("rn2.mid.m") == 40);
    CHECK(ep2->get<int>("rn2.mid.leaf.a") == 12);
}

#if ENABLE_PROTOBUF
// pbtxt 子文件: 子树内容以 protobuf 文本格式落盘/加载
static void test_subtree_pbtxt_file()
{
    const std::string dir = testutil::make_dir("unit_subtree_pbtxt");

    // 写出: 主配置 json, 子文件为 .pbtxt
    auto ep = create<TSubtreeRoot>("pb");
    ep->set("pb.sub.x", "9");
    ep->set("pb.sub.y", "pb");
    ep->set("pb.sub", dir + "/sub.pbtxt");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    ep->save();

    CHECK_STR_CONTAINS(testutil::read_file(dir + "/sub.pbtxt"), "x: 9");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/main.json"), "\"sub\": \"" + dir + "/sub.pbtxt\"");

    // 读回: 新树经主配置里的路径加载 pbtxt 子文件
    auto ep2 = create<TSubtreeRoot>("pb2");
    ep2->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    CHECK(ep2->get<int>("pb2.sub.x") == 9);
    CHECK(ep2->get<std::string>("pb2.sub.y") == "pb");

    // 内容非法 -> 抛 pbtxt 专属异常(与 pbbin/其他格式区分)
    testutil::write_file(dir + "/bad.pbtxt", "not a proto {{{");
    testutil::write_file(dir + "/bad_main.json", "{\"i\":1,\"sub\":\"" + dir + "/bad.pbtxt\"}");
    auto ep3 = create<TSubtreeRoot>("pb3");
    CHECK_THROW({ ep3->set_sink_file(dir + "/bad_main.json", ParameterSerdesType::PST_JSON); }, ParsePbtxtException);
}
#endif

#if ENABLE_PROTOBUF
// pbbin 子文件: 子树内容以 protobuf 二进制格式落盘/加载
static void test_subtree_pbbin_file()
{
    const std::string dir = testutil::make_dir("unit_subtree_pbbin");

    auto ep = create<TSubtreeRoot>("bb");
    ep->set("bb.sub.x", "9");
    ep->set("bb.sub.y", "bin");
    ep->set("bb.sub", dir + "/sub.pbbin");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    ep->save();

    // 落盘的是二进制: 存在且不含可读文本字段
    const auto bin = testutil::read_file(dir + "/sub.pbbin");
    CHECK(!bin.empty());
    CHECK(bin.find("x:") == std::string::npos);
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/main.json"), "sub.pbbin");

    // 读回
    auto ep2 = create<TSubtreeRoot>("bb2");
    ep2->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    CHECK(ep2->get<int>("bb2.sub.x") == 9);
    CHECK(ep2->get<std::string>("bb2.sub.y") == "bin");

    // 非法 wire format -> 抛 pbbin 专属异常(与 pbtxt/其他格式区分)
    testutil::write_file(dir + "/bad.pbbin", std::string("\x08", 1)); // 只有 tag 缺少 varint 值
    testutil::write_file(dir + "/bad_main.json", "{\"i\":1,\"sub\":\"" + dir + "/bad.pbbin\"}");
    auto ep3 = create<TSubtreeRoot>("bb3");
    CHECK_THROW({ ep3->set_sink_file(dir + "/bad_main.json", ParameterSerdesType::PST_JSON); }, ParsePbbinException);
}
#endif

// 子文件缺失属于"配置有误": 不应被当成主配置缺失而用默认值覆盖主文件
static void test_subtree_missing_child()
{
    const std::string dir = testutil::make_dir("unit_subtree_missing_child");

    // 单层: 主文件引用不存在的 sub.json
    testutil::write_file(dir + "/main.json", R"({"i":5,"sub":"/tmp/serdes_test/unit_subtree_missing_child/sub.json"})");
    auto ep = create<TSubtreeRoot>("rm");
    CHECK_THROW({ ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON); }, BadFile);
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/main.json"), "\"i\":5"); // 主配置未被默认值覆盖

    // 嵌套: 中间层存在, 最深层缺失
    testutil::write_file(dir + "/nest_mid.json", R"({"m":1,"leaf":"/tmp/serdes_test/unit_subtree_missing_child/nest_leaf.json"})");
    testutil::write_file(dir + "/nest_main.json", R"({"i":6,"mid":"/tmp/serdes_test/unit_subtree_missing_child/nest_mid.json"})");
    auto epr = create<TNestedRoot>("rmn");
    CHECK_THROW({ epr->set_sink_file(dir + "/nest_main.json", ParameterSerdesType::PST_JSON); }, BadFile);
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/nest_main.json"), "\"i\":6");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/nest_mid.json"), "\"m\":1");
}

// 内存载体(测试注入用): 内容存内存, 不碰磁盘/线程
class MemorySink : public xvorin::serdes::ParameterSink {
public:
    explicit MemorySink(std::string content)
        : content_(std::move(content))
    {
    }

    std::string read() override
    {
        return content_;
    }
    void write(const std::string& c) override
    {
        content_ = c;
    }
    const std::string& id() const override
    {
        return id_;
    }
    ParameterSerdesType format() const override
    {
        return ParameterSerdesType::PST_JSON;
    }
    std::shared_ptr<xvorin::serdes::Monitor> watch(const std::string&, const std::string&,
        std::function<void()>) override
    {
        return nullptr; // 内存载体无需监听
    }

    std::string content_;

private:
    const std::string id_ = "memory:sub";
};

// 注入 ParameterSink: reload 与 save 写回走注入后端, 外部格式/监控不受影响
static void test_subtree_injected_sink()
{
    const std::string dir = testutil::make_dir("unit_subtree_sink");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_sink/sub.json"})");
    CHECK(ep->get<int>("r.sub.x") == 9); // 初始走文件加载

    auto mem = std::make_shared<MemorySink>(R"({"x":11,"y":"mem"})");
    auto* sub = dynamic_cast<xvorin::serdes::Subtree*>(ep->parameter("r.sub").get());
    CHECK(sub != nullptr);
    sub->sink = mem; // 直接替换数据载体

    // reload 走注入 sink: 改内存内容 -> 子树重载 + value_ 同步
    mem->content_ = R"({"x":33,"y":"mem2"})";
    ep->reload_subtree("r.sub", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("r.sub.x") == 33);
    CHECK(ep->get<std::string>("r.sub.y") == "mem2");
    CHECK(ep->value().sub.x == 33);

    // save 写回走注入 sink: 内存内容 = 子树序列化文本(主文件仍是路径)
    ep->set("r.sub.y", "changed");
    ep->save();
    CHECK_STR_CONTAINS(mem->content_, "\"y\": \"changed\"");
    CHECK_STR_CONTAINS(testutil::read_file(dir + "/main.json"), "\"sub\": \"/tmp/serdes_test/unit_subtree_sink/sub.json\"");

    // 恢复默认文件载体: 清空注入, 重新从文件加载
    sub->sink = nullptr;
    ep->reload_subtree("r.sub", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("r.sub.x") == 9);
}

// 主配置 sink 注入内存载体: 参数持久化(load/save)走 ParameterSink 后端
static void test_main_sink_injection()
{
    auto mem = std::make_shared<MemorySink>(R"({"i":5,"s":"frommem","v":[1,2]})");
    auto ep = create<TFileRoot>("ms");
    // 内容格式是载体属性(sink->format()): 不传 sink_type 也能正确加载
    ep->set_sink(mem);
    CHECK(ep->sink_type() == ParameterSerdesType::PST_JSON); // MemorySink::format() 推断
    CHECK(ep->get<int>("ms.i") == 5);
    CHECK(ep->get<std::string>("ms.s") == "frommem");

    // save 写回内存载体
    ep->set("ms.i", "66");
    ep->save();
    CHECK_STR_CONTAINS(mem->content_, "\"i\": 66");

    // 新树从内存内容读回(后端可插拔)
    auto ep2 = create<TFileRoot>("ms2");
    ep2->set_sink(std::make_shared<MemorySink>(mem->content_)); // 同样推断
    CHECK(ep2->get<int>("ms2.i") == 66);
    CHECK(ep2->get<int>("ms2.v.1") == 2);
}

// 细粒度监控: 单个/全部开启、关闭、value_ 同步、save 自激幂等
static void test_subtree_monitor()
{
    const std::string dir = testutil::make_dir("unit_subtree_mon2");
    setup_files(dir);

    auto ep = create<TSubtreeRoot>("r");
    ep->set_sink_file(dir + "/main.json", ParameterSerdesType::PST_JSON);
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_subtree_mon2/sub.json",
        "subs":["/tmp/serdes_test/unit_subtree_mon2/sub2.json"]})");
    CHECK(ep->get<int>("r.sub.x") == 9);

    // 细粒度: 只监听 r.sub
    ep->enable_subtree_monitor("r.sub", true);
    testutil::write_file(dir + "/sub.json", R"({"x":55,"y":"mon"})");
    for (int i = 0; i < 50; i++) {
        usleep(100000);
        if (ep->get<int>("r.sub.x") == 55) {
            break;
        }
    }
    CHECK(ep->get<int>("r.sub.x") == 55);
    CHECK(ep->get<std::string>("r.sub.y") == "mon");
    CHECK(ep->value().sub.x == 55); // reload 后 value_ 同步

    // 未监听的 subs 不重载(子文件仍是加载值)
    CHECK(ep->get<int>("r.subs.0.x") == 42);

    // 关闭细粒度: 不再重载
    ep->enable_subtree_monitor("r.sub", false);
    testutil::write_file(dir + "/sub.json", R"({"x":99,"y":"off"})");
    usleep(300000);
    CHECK(ep->get<int>("r.sub.x") == 55);

    // 全部开启: subs 也被监听
    ep->enable_subtree_monitor(true);
    testutil::write_file(dir + "/sub2.json", R"({"x":77,"y":"s2"})");
    for (int i = 0; i < 50; i++) {
        usleep(100000);
        if (ep->get<int>("r.subs.0.x") == 77) {
            break;
        }
    }
    CHECK(ep->get<int>("r.subs.0.x") == 77);
    CHECK(ep->value().subs[0].x == 77); // 容器内 value_ 同步

    // 监控开启时 save 写回子文件自激 reload: 内容与树一致, 值不变
    ep->save();
    usleep(300000);
    CHECK(ep->get<int>("r.sub.x") == 55);

    ep->enable_subtree_monitor(false);
}

// 自激抑制: 文件内容与最近一次写回/加载一致时, reload 跳过重载(不覆盖内存中的未保存修改)
static void test_subtree_self_suppression()
{
    const std::string dir = testutil::make_dir("unit_subtree_self");
    const std::string child = dir + "/sub.json";
    const std::string main = dir + "/main.json";
    testutil::write_file(child, R"({"x":9,"y":"sub"})");
    testutil::write_file(main, "{\"i\":2,\"sub\":\"" + child + "\"}");

    auto ep = create<TSubtreeRoot>("r");
    ep->set_sink_file(main, ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("r.sub.x") == 9);

    // 内存修改但不落盘; 文件内容未变 -> reload 应被抑制, 内存修改保留
    ep->set("r.sub.x", "100");
    ep->reload_subtree("r.sub", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("r.sub.x") == 100); // 未被文件旧内容覆盖

    // 外部真正改文件 -> 内容变化 -> 正常重载
    testutil::write_file(child, R"({"x":42,"y":"sub"})");
    ep->reload_subtree("r.sub", ParameterSerdesType::PST_JSON);
    CHECK(ep->get<int>("r.sub.x") == 42);
}

int main()
{
    test_single_subtree_json();
    test_subtree_inline();
    test_subtree_env_path();
    test_subtree_other_formats();
    test_subtree_in_containers();
    test_subtree_errors();
    test_subtree_cross_format();
    test_subtree_inline_output();
    test_subtree_path_only();
    test_save_syncs_subtree_file();
    test_subtree_nested();
    test_subtree_self_suppression();
#if ENABLE_PROTOBUF
    test_subtree_pbtxt_file();
    test_subtree_pbbin_file();
#endif
    test_subtree_missing_child();
    test_subtree_monitor();
    test_subtree_injected_sink();
    test_main_sink_injection();
    std::cout << "[OK] subtree" << std::endl;
    return 0;
}
