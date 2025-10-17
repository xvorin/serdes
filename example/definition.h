
#pragma once

#include "serdes/serdes.h"

#include <iostream>
#include <sstream>
#include <string>

#include <list>
#include <map>
#include <memory>
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

enum class ExampleForEnumKey {
    EFEK_A,
    EFEK_B,
    EFEK_C,
    EFEK_D,
    EFEK_E,
};

enum class ExampleForEnumUnderSequence {
    EFEUS_A,
    EFEUS_B,
    EFEUS_C,
};

enum class ExampleForEnumUnderMap {
    EFEUM_A,
    EFEUM_B,
    EFEUM_C,
};

struct ExampleForSmartPtr {
    std::unique_ptr<int> uptri;
    std::shared_ptr<std::string> sptrs;
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
    ExampleForEnum subefe = ExampleForEnum::EFE_B;
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
    ExampleForEnum efe = ExampleForEnum::EFE_D;

    ExampleForInteger efi;
    ExampleSubParameter esp;

    std::vector<int> vi;
    std::list<ExampleSubParameter> lesp;
    std::vector<std::vector<std::list<ExampleForInteger>>> vvlefi;
    std::set<std::string> ss;
    std::map<std::string, std::list<std::string>> msls;
    std::unordered_map<int, ExampleForInheritBase> msefib;

    std::shared_ptr<std::unique_ptr<std::list<std::map<std::string, std::shared_ptr<ExampleForSmartPtr>>>>> sulmse;
    std::unique_ptr<std::list<std::shared_ptr<ExampleForSmartPtr>>> ulmse;

    std::map<ExampleForEnumKey, std::shared_ptr<ExampleForEnum>> mekse;
    std::set<ExampleForEnumUnderSequence> sefeus;
    std::map<ExampleForEnumKey, ExampleForEnumUnderMap> mekes;

    xvorin::serdes::buffer buffer;

    void init_v1();
    void init_v2();
    void init_v3();

    ExampleParameter()
    {
        init_v1();
    }
};
