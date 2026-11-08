#include "definitions.hpp"
#include "util.hpp"

#include <cstring>

using namespace xvorin::serdes;

// protobuf 后端: pbdef 文本/pbtxt/pbbin/pbdbstr 往返、package、subtree 与容器、envar/buffer 映射

static void test_pbdef()
{
    auto ep = create<TBasic>("t");
    const std::string def = ep->to_pbdef();
    CHECK_STR_CONTAINS(def, "syntax = \"proto3\";");
    CHECK_STR_CONTAINS(def, "int32 i8 = 1;");
    CHECK_STR_CONTAINS(def, "double d = 10;");
    CHECK_STR_CONTAINS(def, "bytes buf = 13;"); // buffer -> bytes
    CHECK_STR_CONTAINS(def, "string env = 14;"); // envar -> string

    auto ep2 = create<TStl>("s");
    const std::string def2 = ep2->to_pbdef();
    CHECK_STR_CONTAINS(def2, "repeated int32 vi = 1;");
    CHECK_STR_CONTAINS(def2, "map<string, int32> mi = 3;");
    CHECK_STR_CONTAINS(def2, "repeated string ls = 2;");
}

static void test_pbtxt_pbbin_roundtrip()
{
    auto ep = create<TStl>("s");
    ep->set("s.vi.1", "20");
    ep->set("s.mleaf.L.x", "8");
    ep->create("s.vleaf.2");
    ep->set("s.vleaf.2.x", "5");

    const std::string pbtxt = ep->to_pbtxt();
    CHECK_STR_CONTAINS(pbtxt, "vi: 1\nvi: 20\nvi: 3");

    // pbtxt 往返
    auto ep2 = create<TStl>("s2");
    ep2->from_pbtxt(pbtxt);
    CHECK(ep2->get<int>("s2.vi.1") == 20);
    CHECK(ep2->get<int>("s2.mleaf.L.x") == 8);
    CHECK(ep2->get<int>("s2.vleaf.2.x") == 5);

    // pbbin 往返
    auto ep3 = create<TStl>("s3");
    ep3->from_pbbin(ep->to_pbbin());
    CHECK(ep3->get<int>("s3.vi.1") == 20);
    CHECK(ep3->get<std::string>("s3.mek.RED") == "red");

    // pbdbstr
    CHECK_STR_CONTAINS(ep->to_pbdbstr(), "vi: 1");
}

static void test_pb_package()
{
    auto ep = create<TLeaf>("l");
    ep->set_package("serdes.test");
    CHECK(ep->package() == "serdes.test");
    CHECK_STR_CONTAINS(ep->to_pbdef(), "package serdes.test;");
    // 换 package 后 pbtxt 仍可往返
    const std::string pbtxt = ep->to_pbtxt();
    auto ep2 = create<TLeaf>("l2");
    ep2->set_package("serdes.test");
    ep2->from_pbtxt(pbtxt);
    CHECK(ep2->get<int>("l2.x") == 1);
}

static void test_pb_subtree()
{
    const std::string dir = testutil::make_dir("unit_pb_subtree");
    testutil::write_file(dir + "/sub.json", R"({"x":9,"y":"pb"})");

    auto ep = create<TSubtreeRoot>("r");
    ep->from_json(R"({"i":2,"sub":"/tmp/serdes_test/unit_pb_subtree/sub.json",
        "subs":["/tmp/serdes_test/unit_pb_subtree/sub.json"],
        "submap":{"a":"/tmp/serdes_test/unit_pb_subtree/sub.json"}})");

    // subtree 在 proto 里是 string 字段, 容器生成 repeated string / map<string,string>
    const std::string def = ep->to_pbdef();
    CHECK_STR_CONTAINS(def, "string sub = 2;");
    CHECK_STR_CONTAINS(def, "repeated string subs = 3;");
    CHECK_STR_CONTAINS(def, "map<string, string> submap = 4;");

    // pbtxt: 只传路径, 对端 from_pbtxt 恢复 path
    const std::string pbtxt = ep->to_pbtxt();
    CHECK_STR_CONTAINS(pbtxt, "sub: \"/tmp/serdes_test/unit_pb_subtree/sub.json\"");

    auto ep2 = create<TSubtreeRoot>("r2");
    ep2->from_pbtxt(pbtxt);
    CHECK(ep2->value().sub._subtree_sink_path_ == dir + "/sub.json");
    CHECK(ep2->value().submap.at("a")._subtree_sink_path_ == dir + "/sub.json");
    CHECK(ep2->value().subs.size() == 1 && ep2->value().subs[0]._subtree_sink_path_ == dir + "/sub.json");

    // pbbin 往返同样恢复 path
    auto ep3 = create<TSubtreeRoot>("r3");
    ep3->from_pbbin(ep->to_pbbin());
    CHECK(ep3->value().sub._subtree_sink_path_ == dir + "/sub.json");

    // 内联模式: protobuf 生成嵌套 message(subtree_inline), 输出子树内容对象而非路径
    ep->enable_subtree_inline(true);
    const std::string def_inline = ep->to_pbdef();
    CHECK_STR_CONTAINS(def_inline, "message TLeaf {");
    CHECK_STR_CONTAINS(def_inline, "TLeaf sub = 2;");
    CHECK_STR_CONTAINS(def_inline, "repeated TLeaf subs = 3;");
    CHECK_STR_CONTAINS(def_inline, "map<string, TLeaf> submap = 4;");

    const std::string pbtxt_inline = ep->to_pbtxt();
    CHECK_STR_CONTAINS(pbtxt_inline, "sub {\n  x: 9"); // 子树内容以内嵌 message 输出
    CHECK_STR_CONTAINS(pbtxt_inline, "subs {\n  x: 9");

    // 内联往返: 对端同样开启 inline 读回子树内容
    auto ep4 = create<TSubtreeRoot>("r4");
    ep4->enable_subtree_inline(true);
    ep4->from_pbtxt(pbtxt_inline);
    CHECK(ep4->get<int>("r4.sub.x") == 9);
    CHECK(ep4->get<int>("r4.subs.0.x") == 9);
    CHECK(ep4->get<int>("r4.submap.a.x") == 9);

    // 关闭: schema 重建回 string 字段 + 路径传输
    ep->enable_subtree_inline(false);
    CHECK_STR_CONTAINS(ep->to_pbdef(), "string sub = 2;");
    CHECK_STR_CONTAINS(ep->to_pbtxt(), "sub: \"/tmp/serdes_test/unit_pb_subtree/sub.json\"");
}

int main()
{
    test_pbdef();
    test_pbtxt_pbbin_roundtrip();
    test_pb_package();
    test_pb_subtree();
    std::cout << "[OK] protobuf" << std::endl;
    return 0;
}
