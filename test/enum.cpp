#include "definitions.hpp"
#include "util.hpp"

using namespace xvorin::serdes;

// enum 容器承载类型(本 TU 专用)
struct EHolder {
    std::vector<TColor> vc;
    std::set<TColor> sc;
};
DEFINE_PARAM(EHolder, vc, "vc")
DEFINE_PARAM(EHolder, sc, "sc")

// enum: 名字映射、各格式序列化、非法值、作为容器元素与 map key

static void test_enum_basic()
{
    auto ep = create<TStl>("t");
    // 序列化为名字
    CHECK_STR_CONTAINS(ep->to_json(), "RED");
    // 反序列化名字 -> 枚举值(map<TColor,string> 的 value 是 string, 取 value 断言)
    auto ep2 = create<TStl>("t2");
    ep2->from_json(ep->to_json());
    CHECK(ep2->get<std::string>("t2.mek.RED") == "red");
    CHECK_STR_CONTAINS(ep2->debug_string(), "t2.mek.RED = red [std::string]");
}

static void test_enum_under_containers()
{
    // vector<enum> / set<enum> 往返
    auto ep = create<EHolder>("h");
    ep->from_json(R"({"vc":["RED","GREEN"],"sc":["BLUE"]})");

    const std::string j = ep->to_json();
    CHECK_STR_CONTAINS(j, "\"RED\"");
    CHECK_STR_CONTAINS(j, "\"BLUE\"");

    auto ep2 = create<EHolder>("h2");
    ep2->from_json(j);
    CHECK(ep2->get<TColor>("h2.vc.1") == TColor::GREEN);

    auto ep3 = create<EHolder>("h3");
    ep3->from_yaml(ep->to_yaml());
    CHECK(ep3->get<TColor>("h3.sc.0") == TColor::BLUE);

    // 非法枚举名 -> InvalidValueException
    CHECK_THROW(ep3->set("h3.vc.0", "PURPLE"), InvalidValueException);
}

int main()
{
    test_enum_basic();
    test_enum_under_containers();
    std::cout << "[OK] enum" << std::endl;
    return 0;
}
