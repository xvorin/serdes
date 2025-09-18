
#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include <list>
#include <set>
#include <unordered_map>
#include <vector>

struct BaseParam {
    int v1 = 1;
    double v2 = 5;
};

struct SubParam : public BaseParam {
    std::string v3 = "123";

    std::string show()
    {
        std::stringstream ss;
        ss << "v1:" << v1 << std::endl;
        ss << "      v2:" << v2 << std::endl;
        ss << "      v3:" << v3;
        return ss.str();
    }
};

enum class TestEnum {
    TE_A,
    TE_B,
    TE_C,
};

struct Param {
    int x;
    std::string y;
    bool z;
    double h;

    TestEnum e;

    std::vector<int> o;
    std::list<SubParam> n;
    std::vector<std::vector<std::list<SubParam>>> m;
    std::set<std::string> p;
    std::unordered_map<std::string, SubParam> q;

    SubParam r;

    Param()
    {
        x = 1;
        y = "123-12345678";
        z = false;
        e = TestEnum::TE_B;

        o = { 1, 2, 3, 4, 5, 6 };
        n.resize(2);

        m.resize(2);
        m[0].resize(1);
        m[0][0].resize(1);

        p = { "Hello", "World" };
        q = { { "mk1", SubParam() }, { "mk2", SubParam() } };
    }

    std::string show()
    {
        std::stringstream ss;
        ss << "x:" << x << std::endl;
        ss << "y:" << y << std::endl;
        ss << "z:" << z << std::endl;
        ss << "e:" << static_cast<int>(e) << std::endl;

        ss << "o:";
        for (auto& i : o) {
            ss << i << ",";
        }
        ss << std::endl;

        ss << "p:";
        for (auto& i : p) {
            ss << i << ",";
        }
        ss << std::endl;

        ss << "q:";
        for (auto& i : q) {
            ss << std::endl
               << "  " << i.first << ":" << i.second.show();
        }

        return ss.str();
    }
};
