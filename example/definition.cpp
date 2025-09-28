#include "definition.h"
#include "serdes/serdes.h"

// 注册成员
// DEFINE_PARAM(ExampleForInheritBase, v1, "测试commont1", nullptr, "[1,5);(4,3)");
// DEFINE_PARAM(ExampleForInheritBase, v2, "测试commont2", nullptr, "(0,5.8]");
// DEFINE_PARAM(ExampleSubParameter, v3, "测试commont3", nullptr, "^\\d{3}$");

// DEFINE_PARAM(ExampleParameter, x, "整型变量", nullptr);
// DEFINE_PARAM(ExampleParameter, y, "电话号码", nullptr, "^\\d{3}-\\d{8}$");
// DEFINE_PARAM(ExampleParameter, z, "布尔变量");
// DEFINE_PARAM(ExampleParameter, h, "浮点变量");
// DEFINE_PARAM(ExampleParameter, e, "enum变量");

// DEFINE_PARAM(ExampleParameter, o);
// DEFINE_PARAM(ExampleParameter, n);
// DEFINE_PARAM(ExampleParameter, m);
// DEFINE_PARAM(ExampleParameter, p);
// DEFINE_PARAM(ExampleParameter, q);
// DEFINE_PARAM(ExampleParameter, r);

// // 注册继承关系
// DEFINE_INHERIT(ExampleForInheritBase, ExampleSubParameter);

// // 注册根节点(可选)
// DEFINE_ROOT(ExampleParameter);

DEFINE_PARAM(ExampleForInteger, i8, "c for i8")
DEFINE_PARAM(ExampleForInteger, i16, "c for i16")
DEFINE_PARAM(ExampleForInteger, i32, "c for i32")
DEFINE_PARAM(ExampleForInteger, i64, "c for i64")
DEFINE_PARAM(ExampleForInteger, ui8, "c for ui8")
DEFINE_PARAM(ExampleForInteger, ui16, "c for ui16")
DEFINE_PARAM(ExampleForInteger, ui32, "c for ui32")
DEFINE_PARAM(ExampleForInteger, ui64, "c for ui64")

DEFINE_PARAM(ExampleForInheritBase, i, "c for i2")
DEFINE_PARAM(ExampleForInheritBase, d, "c for d2")
DEFINE_PARAM(ExampleForInheritBase, s, "c for s2")

DEFINE_INHERIT(ExampleForInheritBase, ExampleSubParameter);

DEFINE_PARAM(ExampleSubParameter, subs, "c for subs")
DEFINE_PARAM(ExampleSubParameter, subb, "c for subb")
DEFINE_PARAM(ExampleSubParameter, subd, "c for subd")
DEFINE_PARAM(ExampleSubParameter, subf, "c for subf")
DEFINE_PARAM(ExampleSubParameter, subefe, "c for subefei")
DEFINE_PARAM(ExampleSubParameter, subefi, "c for subefi")

DEFINE_PARAM(ExampleParameter, i, "c for i")
DEFINE_PARAM(ExampleParameter, b, "c for b")
DEFINE_PARAM(ExampleParameter, d, "c for d")
DEFINE_PARAM(ExampleParameter, f, "c for f")
DEFINE_PARAM(ExampleParameter, s, "c for s")
DEFINE_PARAM(ExampleParameter, efe, "c for efe")
DEFINE_PARAM(ExampleParameter, efi, "c for efi")
DEFINE_PARAM(ExampleParameter, esp, "c for esp")
DEFINE_PARAM(ExampleParameter, vi, "c for vi")
DEFINE_PARAM(ExampleParameter, lesp, "c for lesp")
DEFINE_PARAM(ExampleParameter, vvlefi, "c for vvlefi")
DEFINE_PARAM(ExampleParameter, ss, "c for ss")
DEFINE_PARAM(ExampleParameter, msls, "c for msls")
DEFINE_PARAM(ExampleParameter, msefib, "c for msefib")

DEFINE_ROOT(ExampleParameter, "c for ExampleParameter");
DEFINE_ROOT(ExampleForInheritBase, "c for ExampleForInheritBase");
DEFINE_ROOT(ExampleSubParameter, "c for ExampleSubParameter");
DEFINE_ROOT(ExampleForInteger, "c for ExampleForInteger");

void ExampleForInteger::init_v1()
{
    i8 = -1;
    i16 = -2;
    i32 = -3;
    i64 = -4;
    ui8 = 1;
    ui16 = 2;
    ui32 = 3;
    ui64 = 4;
}

void ExampleForInteger::init_v2()
{
    i8 = -5;
    i16 = -6;
    i32 = -7;
    i64 = -8;
    ui8 = 5;
    ui16 = 6;
    ui32 = 7;
    ui64 = 8;
}

void ExampleForInteger::init_v3()
{
    i8 = -11;
    i16 = -12;
    i32 = -13;
    i64 = -14;
    ui8 = 11;
    ui16 = 12;
    ui32 = 13;
    ui64 = 14;
}

void ExampleForInheritBase::init_v1()
{
    i = 123;
    d = 45.67;
    s = "";
}
void ExampleForInheritBase::init_v2()
{
    i = 456;
    d = 78.91;
    s = "efib2";
}
void ExampleForInheritBase::init_v3()
{
    i = 789;
    d = 12.34;
    s = "efib3";
}

void ExampleSubParameter::init_v1()
{
    subs = "esp1";
    subb = false;
    subd = 19.56;
    subf = 9.12;
    subefe = ExampleForEnum::EFE_C;
    subefi.init_v1();
}
void ExampleSubParameter::init_v2()
{
    subs = "esp2";
    subb = true;
    subd = 219.56;
    subf = 29.12;
    subefe = ExampleForEnum::EFE_D;
    subefi.init_v2();
}
void ExampleSubParameter::init_v3()
{
    subs = "esp3";
    subb = false;
    subd = 319.56;
    subf = 39.12;
    subefe = ExampleForEnum::EFE_E;
    subefi.init_v3();
}

void ExampleParameter::init_v1()
{
    i = 1;
    b = true;
    d = 5.6;
    f = 2.0;
    s = "Hi!";
    efe = ExampleForEnum::EFE_B;

    efi.init_v1();
    esp.init_v1();

    vi = { 1, 2, 3 };

    lesp.clear();
    lesp.emplace_back();
    lesp.back().init_v1();

    vvlefi.clear();
    vvlefi.emplace_back();
    vvlefi.back().emplace_back();
    vvlefi.back().back().emplace_back();
    vvlefi.back().back().back().init_v1();

    ss = { "A", "B", "C" };

    msls = { { "MSLS1", { "LS1", "LS2" } } };
    msefib["MSEFIB1"].init_v1();
}

void ExampleParameter::init_v2()
{
    i = 8;
    b = false;
    d = 7.9;
    f = 5.1;
    s = "Hi?";
    efe = ExampleForEnum::EFE_A;

    efi.init_v2();
    esp.init_v2();

    vi = { 6, 7, 8, 9 };

    lesp.clear();
    lesp.emplace_back();
    lesp.back().init_v2();
    lesp.emplace_back();
    lesp.back().init_v2();

    vvlefi.clear();
    vvlefi.emplace_back();
    vvlefi.back().emplace_back();
    vvlefi.back().back().emplace_back();
    vvlefi.back().back().back().init_v2();

    ss = { "D", "E", "F", "K" };

    msls = { { "MSLS_A", { "LS1", "LS2" } }, { "MSLS_B", { "LS3", "LS4" } } };
    msefib["MSEFIB2"].init_v2();
}

void ExampleParameter::init_v3()
{
    i = 18;
    b = false;
    d = 17.9;
    f = 15.1;
    s = "Hi~";
    efe = ExampleForEnum::EFE_E;

    efi.init_v3();
    esp.init_v3();

    vi = { 6, 7, 8, 9, 0 };

    lesp.clear();
    lesp.emplace_back();
    lesp.back().init_v3();

    lesp.emplace_back();
    lesp.back().init_v3();

    vvlefi.clear();
    vvlefi.emplace_back();
    vvlefi.back().emplace_back();
    vvlefi.back().back().emplace_back();
    vvlefi.back().back().back().init_v3();

    vvlefi.emplace_back();
    vvlefi.back().emplace_back();
    vvlefi.back().back().emplace_back();
    vvlefi.back().back().back().init_v3();

    ss = { "A", "D", "E", "F", "K" };

    msls = { { "MSLS_B", { "LS3", "LS4" } } };
    msefib["MSEFIB3"].init_v3();
}

std::string show()
{
    return "";
}
