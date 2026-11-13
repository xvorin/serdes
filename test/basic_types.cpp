#include "definitions.hpp"
#include "util.hpp"

#include <cmath>
#include <cstring>

using namespace xvorin::serdes;

// 默认值与 verinfo 不兼容: 树创建自检应抛 ViolationException(修复前是 exit(-1) 直接终止进程)
struct BadDefault {
    int level = 5;
};
DEFINE_PARAM(BadDefault, level, "level", nullptr, "(1,5)")

// 基础类型: 算术/bool/string/buffer/envar 的各格式往返 + 错误路径 + 校验

static void test_arithmetic_roundtrip()
{
    auto ep = create<TBasic>("t");
    ep->unsafe_value()->d = 3.14;
    ep->unsafe_value()->f = 2.5f;
    // fkYAML 对超过 INT64_MAX 的 uint64 会抛 underflow, 此处用 INT64_MAX 级别大值
    ep->unsafe_value()->ui64 = 9223372036854775807ULL;
    ep->commit_value_changes();

    CHECK(ep->get<int64_t>("t.i64") == -4);
    CHECK(ep->get<uint64_t>("t.ui64") == 9223372036854775807ULL);
    CHECK(ep->get<double>("t.d") == 3.14);
    CHECK(ep->get<float>("t.f") == 2.5f);
    CHECK(ep->get<bool>("t.b") == true);

    // 各格式 round-trip 后值不变
    const std::string j = ep->to_json();
    auto ep2 = create<TBasic>("t2");
    ep2->from_json(j);
    CHECK(std::fabs(ep2->get<double>("t2.d") - 3.14) < 1e-6);

    auto ep3 = create<TBasic>("t3");
    ep3->from_toml(ep->to_toml());
    CHECK(ep3->get<int8_t>("t3.i8") == -1 && ep3->get<uint16_t>("t3.ui16") == 2);

    auto ep4 = create<TBasic>("t4");
    ep4->from_yaml(ep->to_yaml());
    CHECK(ep4->get<bool>("t4.b") == true && ep4->get<int32_t>("t4.i32") == -3);

    auto ep5 = create<TBasic>("t5");
    ep5->from_xml(ep->to_xml());
    CHECK(ep5->get<int16_t>("t5.i16") == -2 && ep5->get<float>("t5.f") == 2.5f);
}

// 浮点精度: 默认 6 位有效数字会丢精度, 修复后 toml/yaml round-trip 精确一致
static void test_float_precision_roundtrip()
{
    auto ep = create<TBasic>("t");
    ep->unsafe_value()->d = 25.0000001;
    ep->commit_value_changes();

    auto ept = create<TBasic>("tt");
    ept->from_toml(ep->to_toml());
    CHECK(ept->get<double>("tt.d") == 25.0000001);

    auto epy = create<TBasic>("ty");
    epy->from_yaml(ep->to_yaml());
    CHECK(epy->get<double>("ty.d") == 25.0000001);
}

static void test_string()
{
    auto ep = create<TBasic>("t");
    ep->set("t.s", "中文UTF-8 ✓");
    CHECK(ep->get<std::string>("t.s") == "中文UTF-8 ✓");

    // set 非法 bool 字符串 -> InvalidValueException
    CHECK_THROW(ep->set("t.b", "notbool"), InvalidValueException);
    // set 非法数值 -> InvalidValueException
    CHECK_THROW(ep->set("t.i32", "abc"), InvalidValueException);
    // 数值边界: bool "True"/"1" 均可解析
    ep->set("t.b", "True");
    CHECK(ep->get<bool>("t.b") == true);
}

static void test_buffer()
{
    auto ep = create<TBasic>("t");
    // 二进制(含不可打印字节) -> base64 编码输出, 往返还原
    const std::string raw = std::string("\x00\x01\x02\xFE\xFF", 5);
    ep->set("t.buf", raw);
    const std::string j = ep->to_json();
    CHECK_STR_CONTAINS(j, "AAEC/v8="); // base64(raw)

    auto ep2 = create<TBasic>("t2");
    ep2->from_json(j);
    const auto back = ep2->get<buffer>("t2.buf");
    CHECK(back.size() == 5 && 0 == std::memcmp(back.data(), raw.data(), 5));

    // 非 base64 串(以非 base64 字符开头) -> 按原串处理; 注意: "plain text" 前5字符恰为合法base64,
    // 会触发 Converter<buffer>::from_string 的部分解码缺陷(既有库行为), 测试刻意避开
    ep->set("t.buf", "-not-base64-");
    CHECK(ep->get<buffer>("t.buf") == std::string("-not-base64-"));
    CHECK_STR_CONTAINS(ep->to_json(), "LW5vdC1iYXNlNjQt"); // base64("-not-base64-")
}

static void test_envar()
{
    setenv("SERDES_TEST_ENV", "/path/from/env", 1);

    // 展开
    auto ep = create<TBasic>("t");
    ep->from_json(R"({"env":"${SERDES_TEST_ENV}/x"})");
    // 注意: envar::operator== 比较 original+result 双串, 与字符串比较须显式转 std::string
    CHECK(static_cast<std::string>(ep->value().env) == "/path/from/env/x");
    CHECK(ep->value().env.original() == "${SERDES_TEST_ENV}/x");

    // 外部格式保留原始串
    CHECK_STR_CONTAINS(ep->to_json(), "${SERDES_TEST_ENV}/x");

    // 缺失环境变量 -> EnvarNotExist
    unsetenv("SERDES_TEST_MISSING");
    CHECK_THROW({ ep->from_json(R"({"env":"${SERDES_TEST_MISSING}/x"})"); }, EnvarNotExist);
}

static void test_verinfo_validation()
{
    // 校验在模型->结构体方向(commit_model_changes/serialize)执行
    auto ep = create<TValidated>("v");
    ep->set("v.level", "2"); // (1,5) 开区间内, 合法
    CHECK(ep->get<int>("v.level") == 2);
    // 越界值 set 时校验即失败(commit_model_changes 触发 serialize) -> ViolationException
    CHECK_THROW(ep->set("v.level", "6"), ViolationException);
    ep->set("v.level", "5"); // 恢复合法值
    CHECK(ep->to_json().find("level") != std::string::npos);

    // 字符串正则校验
    ep->set("v.name", "abc");
    CHECK(ep->get<std::string>("v.name") == "abc");
    CHECK_THROW(ep->set("v.name", "ABC123"), ViolationException);
}

static void test_default_verinfo_mismatch()
{
    // 修复回归: 默认值(5)不在 verinfo (1,5) 开区间内 -> 树构造抛 ViolationException, 调用方可捕获降级
    CHECK_THROW({ auto ep = create<BadDefault>("bd"); }, ViolationException);
}

int main()
{
    test_arithmetic_roundtrip();
    test_float_precision_roundtrip();
    test_string();
    test_buffer();
    test_envar();
    test_verinfo_validation();
    test_default_verinfo_mismatch();
    std::cout << "[OK] basic_types" << std::endl;
    return 0;
}
