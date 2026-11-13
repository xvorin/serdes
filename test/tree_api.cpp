#include "definitions.hpp"
#include "util.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace xvorin::serdes;

// 参数树 API: exist/parameter/get/set/create/remove/debug_string/命令行/变更通知/双向同步

static void test_get_set_exist()
{
    auto ep = create<TFileRoot>("r");
    CHECK(ep->exist("r.i") != nullptr);
    CHECK(nullptr == ep->exist("r.nope"));

    // 索引不存在 -> ParameterNotFound; 空索引 -> InvalidIndex
    CHECK_THROW(ep->get<int>("r.nope"), ParameterNotFound);
    CHECK_THROW(ep->parameter(""), InvalidIndex);

    // 通过 parameter<T> 只读访问 value; 修改走 set
    CHECK(ep->parameter<int>("r.i")->value == 1);
    ep->set("r.i", "99");
    CHECK(ep->value().i == 99);

    // 字符串形式 set(走 from_string)
    ep->set("r.s", "hello");
    CHECK(ep->get<std::string>("r.s") == "hello");
    ep->set("r.s", "hello world");
    CHECK(ep->get<std::string>("r.s") == "hello world");

    // set 后 value() 同步
    ep->set("r.i", "42");
    CHECK(ep->value().i == 42);
}

static void test_value_bidirectional_sync()
{
    auto ep = create<TFileRoot>("r");
    // value -> model
    ep->unsafe_value()->v = { 10, 20, 30 };
    ep->commit_value_changes();
    CHECK(ep->get<int>("r.v.2") == 30);
    // model -> value
    ep->set("r.v.1", "21");
    CHECK(ep->value().v[1] == 21);
}

static void test_command_line()
{
    auto ep = create<TFileRoot>("r");
    const std::string dir = testutil::make_dir("unit_cmd");
    const std::string sink = dir + "/cmd.toml";

    char a0[] = "test_serdes";
    char a1[] = "--r.i=77";
    char a2[] = "--r.s=x";
    char a3[] = "--create";
    char a4[] = "r.v.2";
    char a5[] = "--r.v.2=9";
    char a6[] = "--save";
    char* argv[] = { a0, a1, a2, a3, a4, a5, a6 };
    ep->set_sink_file(sink, ParameterSerdesType::PST_TOML);
    ep->parse_command_line(7, argv);

    CHECK(ep->get<int>("r.i") == 77);
    CHECK(ep->get<std::string>("r.s") == "x");
    CHECK(ep->get<int>("r.v.2") == 9);
    CHECK(ep->value().v.size() == 3);
    // save 已写入文件, 读回一致
    const std::string content = testutil::read_file(sink);
    CHECK_STR_CONTAINS(content, "i = 77");
}

static void test_inform_notifications()
{
    // 带变更回调的根: 回调记录 set 前后值
    static std::vector<std::string> notified;
    struct NotifyRoot {
        int x = 1;
    };
    DEFINE_PARAM(NotifyRoot, x, "x",
        [](const std::string& index, int oldv, int newv) {
            notified.push_back(index + ":" + std::to_string(oldv) + "->" + std::to_string(newv));
            return true;
        });

    auto ep = create<NotifyRoot>("nr");
    ep->enable_inform(true);
    ep->set("nr.x", "5");
    CHECK(!notified.empty() && notified.back() == "nr.x:1->5");

    // inform 返回 false -> 拒绝变更并记 violation
    struct RejectRoot {
        int y = 0;
    };
    DEFINE_PARAM(RejectRoot, y, "y",
        [](const std::string&, int, int) { return false; });

    auto ep2 = create<RejectRoot>("rr");
    ep2->enable_inform(true);
    // 被回调拒绝 -> set 时即 ViolationException; model 侧已置值但未落入结构体
    CHECK_THROW(ep2->set("rr.y", "3"), ViolationException);
    CHECK(ep2->get<int>("rr.y") == 3); // 参数节点值已写入
    CHECK(ep2->value().y == 0); // 结构体侧未被接受
}

// 指针成员 + inform: 指针内子节点事件应沿父链上抛到指针节点注册的回调(3 参: 带 node)
static std::vector<std::string> g_ptr_events;
static void on_ptr_inform(const std::string& index, ParameterInformType pit, const std::shared_ptr<Parameter>& node)
{
    const char* name = (pit == ParameterInformType::PIT_CREATE) ? "create"
        : (pit == ParameterInformType::PIT_REMOVE)              ? "remove"
                                                                : "change";
    g_ptr_events.push_back(std::string(name) + ":" + index + (node ? ":node" : ":null"));
}

struct PtrInner {
    int v = 1;
};
struct PtrRoot {
    std::shared_ptr<PtrInner> inner = std::make_shared<PtrInner>();
};
DEFINE_PARAM(PtrInner, v, "v")
DEFINE_PARAM(PtrRoot, inner, "指针", on_ptr_inform)

static void test_ptr_inform_propagation()
{
    g_ptr_events.clear();
    auto ep = create<PtrRoot>("pr");
    ep->enable_inform(true);
    ep->set("pr.inner.0.v", "2"); // 指针内子节点用 0 键; PIT_CHANGE 从 v 上抛到 inner 指针的 inform
    CHECK(!g_ptr_events.empty());
    CHECK_STR_CONTAINS(g_ptr_events.back(), "change:pr.inner.0.v:node");
}

// 容器元素增删: PIT_CREATE 带节点, PIT_REMOVE 节点为 null
static std::vector<std::string> g_map_events;
static void on_map_inform(const std::string& index, ParameterInformType pit, const std::shared_ptr<Parameter>& node)
{
    const char* name = (pit == ParameterInformType::PIT_CREATE) ? "create"
        : (pit == ParameterInformType::PIT_REMOVE)              ? "remove"
                                                                : "change";
    g_map_events.push_back(std::string(name) + ":" + index + (node ? ":node" : ":null"));
}

struct MapRoot {
    std::map<int, int> m;
};
DEFINE_PARAM(MapRoot, m, "map", on_map_inform)

static void test_container_inform_node()
{
    g_map_events.clear();
    auto ep = create<MapRoot>("rm");
    ep->enable_inform(true);
    ep->create("rm.m.5");
    CHECK(g_map_events.size() == 1);
    CHECK_STR_CONTAINS(g_map_events[0], "create:rm.m.5:node");

    ep->remove("rm.m.5");
    CHECK(g_map_events.size() == 2);
    CHECK_STR_CONTAINS(g_map_events[1], "remove:rm.m.5:null");
}

// set 元素不是独立节点: 增删事件 node 均为 null, 索引为容器索引
static std::vector<std::string> g_set_events;
static void on_set_inform(const std::string& index, ParameterInformType pit, const std::shared_ptr<Parameter>& node)
{
    const char* name = (pit == ParameterInformType::PIT_CREATE) ? "create"
        : (pit == ParameterInformType::PIT_REMOVE)              ? "remove"
                                                                : "change";
    g_set_events.push_back(std::string(name) + ":" + index + (node ? ":node" : ":null"));
}

struct SetRoot {
    std::set<int> s;
};
DEFINE_PARAM(SetRoot, s, "set", on_set_inform)

static void test_set_inform_node()
{
    g_set_events.clear();
    auto ep = create<SetRoot>("rs");
    ep->enable_inform(true);
    ep->create("rs.s.0");
    CHECK(g_set_events.size() == 1);
    CHECK_STR_CONTAINS(g_set_events[0], "create:rs.s:null");

    ep->remove("rs.s.0");
    CHECK(g_set_events.size() == 2);
    CHECK_STR_CONTAINS(g_set_events[1], "remove:rs.s:null");
}

static void test_debug_string()
{
    auto ep = create<TFileRoot>("r");
    const std::string dbg = ep->debug_string();
    CHECK_STR_CONTAINS(dbg, "|r.i = 1 [int]");
    CHECK_STR_CONTAINS(dbg, "|r.v size = 2 [int]");
    // 子树 debug
    CHECK_STR_CONTAINS(ep->debug_string("r.v"), "r.v.0 = 1 [int]");
    CHECK_THROW(ep->debug_string("r.nope"), ParameterNotFound);
}

int main()
{
    test_get_set_exist();
    test_value_bidirectional_sync();
    test_command_line();
    test_inform_notifications();
    test_ptr_inform_propagation();
    test_container_inform_node();
    test_set_inform_node();
    test_debug_string();
    std::cout << "[OK] tree_api" << std::endl;
    return 0;
}
