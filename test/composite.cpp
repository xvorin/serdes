#include "definitions.hpp"
#include "util.hpp"

#include <cstring>

using namespace xvorin::serdes;

// 复合类型: 嵌套对象/继承/STL 容器/嵌套 STL/智能指针 的各格式往返 + 运行时增删

static void test_nested_object()
{
    auto ep = create<TNode>("n");
    ep->set("n.level", "3");
    ep->set("n.leaf.x", "9");
    CHECK(ep->get<int>("n.leaf.x") == 9 && ep->get<int>("n.level") == 3);
    CHECK_STR_CONTAINS(ep->debug_string(), "n.leaf.x = 9 [int]");

    // 嵌套往返
    auto ep2 = create<TNode>("n2");
    ep2->from_json(ep->to_json());
    CHECK(ep2->get<std::string>("n2.leaf.y") == "y");

    auto ep3 = create<TNode>("n3");
    ep3->from_yaml(ep->to_yaml());
    CHECK(ep3->get<int>("n3.leaf.x") == 9);
}

static void test_inheritance()
{
    auto ep = create<TDerived>("d");
    // 基类成员与派生成员在同一子树
    CHECK(ep->get<int>("d.bi") == 10);
    CHECK(ep->get<std::string>("d.bs") == "bs");
    CHECK(ep->get<std::string>("d.ds") == "ds");
    CHECK(ep->get<int>("d.dl.x") == 1);
    CHECK_STR_CONTAINS(ep->debug_string(), "[TDerived:TBase]");

    // 继承往返(各格式)
    auto ep2 = create<TDerived>("d2");
    ep2->from_json(ep->to_json());
    CHECK(ep2->get<int>("d2.bi") == 10 && ep2->get<std::string>("d2.ds") == "ds");
    auto ep3 = create<TDerived>("d3");
    ep3->from_toml(ep->to_toml());
    CHECK(ep3->get<std::string>("d3.bs") == "bs");
}

static void test_stl_containers()
{
    auto ep = create<TStl>("s");
    CHECK(ep->get<int>("s.vi.0") == 1 && ep->get<int>("s.vi.2") == 3);
    CHECK(ep->get<std::string>("s.ls.1") == "b");
    CHECK(ep->get<int>("s.mi.one") == 1);
    CHECK(ep->get<std::string>("s.umis.2") == "u2");
    CHECK(ep->get<int>("s.vvi.1.0") == 3);
    CHECK(ep->get<int>("s.mvi.k.1") == 8);
    CHECK(ep->get<int>("s.vleaf.1.x") == 2);
    CHECK(ep->get<std::string>("s.mleaf.L.y") == "z");
    CHECK(ep->get<std::string>("s.mek.RED") == "red");

    // 各格式往返
    for (const auto& fmt : { "json", "yaml", "toml", "xml" }) {
        auto ep2 = create<TStl>("s2");
        if (0 == std::strcmp(fmt, "json")) {
            ep2->from_json(ep->to_json());
        } else if (0 == std::strcmp(fmt, "yaml")) {
            ep2->from_yaml(ep->to_yaml());
        } else if (0 == std::strcmp(fmt, "toml")) {
            ep2->from_toml(ep->to_toml());
        } else {
            ep2->from_xml(ep->to_xml());
        }
        CHECK_MSG(ep2->get<int>("s2.vi.1") == 2 && ep2->get<std::string>("s2.ls.0") == "a"
                && ep2->get<int>("s2.mi.two") == 2 && ep2->get<int>("s2.vvi.0.1") == 2,
            std::string("stl roundtrip via ") + fmt);
    }
}

static void test_stl_create_remove()
{
    auto ep = create<TStl>("s");
    // vector 尾部追加
    ep->create("s.vi.3");
    ep->set("s.vi.3", "4");
    CHECK(ep->get<int>("s.vi.3") == 4);
    // vector 中间插入: 新元素占位, 原元素后移
    ep->create("s.vi.1");
    CHECK(ep->get<int>("s.vi.2") == 2); // 原 1 号后移
    // map 增删
    ep->create("s.mi.three");
    ep->set("s.mi.three", "3");
    CHECK(ep->get<int>("s.mi.three") == 3);
    ep->remove("s.mi.two");
    CHECK(nullptr == ep->exist("s.mi.two"));
    // 重复创建 -> IndexDuplicate; 删除不存在 -> ParameterNotFound
    CHECK_THROW(ep->create("s.mi.three"), IndexDuplicate);
    CHECK_THROW(ep->remove("s.mi.nope"), ParameterNotFound);
}

static void test_smart_ptr()
{
    // 智能指针内部子节点挂在 "0" 索引层下(pointer.hpp 的设计)
    auto ep = create<TPtr>("p");
    CHECK(ep->get<int>("p.sp.0.x") == 3);
    CHECK(ep->get<int>("p.up.0.x") == 4);
    CHECK(ep->debug_string().find("nullsp = nullptr") != std::string::npos);

    // 各格式往返(含 nullptr 字段)
    auto ep2 = create<TPtr>("p2");
    ep2->from_json(ep->to_json());
    CHECK(ep2->get<int>("p2.sp.0.x") == 3);
    CHECK(ep2->debug_string().find("nullsp = nullptr") != std::string::npos); // null 字段保持空

    // set 修改指针内容
    ep->set("p.up.0.x", "40");
    CHECK(ep->get<int>("p.up.0.x") == 40);
}

int main()
{
    test_nested_object();
    test_inheritance();
    test_stl_containers();
    test_stl_create_remove();
    test_smart_ptr();
    std::cout << "[OK] composite" << std::endl;
    return 0;
}
