#include "definitions.hpp"
#include "util.hpp"

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
    test_debug_string();
    std::cout << "[OK] tree_api" << std::endl;
    return 0;
}
