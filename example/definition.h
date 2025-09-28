
#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

struct ExampleForInteger {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;

    uint8_t ui8;
    uint16_t ui16;
    uint32_t ui32;
    uint64_t ui64;

    void init_v1();
    void init_v2();
    void init_v3();
};

enum class ExampleForEnum {
    EFE_A,
    EFE_B,
    EFE_C,
    EFE_D,
    EFE_E,
};

struct ExampleForInheritBase {
    int i = 1;
    double d;
    std::string s;

    void init_v1();
    void init_v2();
    void init_v3();
};

struct ExampleSubParameter : public ExampleForInheritBase {
    std::string subs;
    bool subb;
    double subd;
    float subf;
    ExampleForEnum subefe;
    ExampleForInteger subefi;

    void init_v1();
    void init_v2();
    void init_v3();
};

struct ExampleParameter {
    int i;
    bool b;
    double d;
    float f;
    std::string s;
    ExampleForEnum efe;

    ExampleForInteger efi;
    ExampleSubParameter esp;

    std::vector<int> vi;
    std::list<ExampleSubParameter> lesp;
    std::vector<std::vector<std::list<ExampleForInteger>>> vvlefi;
    std::set<std::string> ss;
    std::map<std::string, std::list<std::string>> msls;
    std::unordered_map<std::string, ExampleForInheritBase> msefib;

    void init_v1();
    void init_v2();
    void init_v3();

    std::string show();

    ExampleParameter()
    {
        init_v1();
    }
};
