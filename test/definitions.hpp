#pragma once

#include "serdes/serdes.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ---- 公共测试类型: 注册位于 definitions.cpp(独立TU), 与 example 的跨TU注册模式一致 ----

// 全部基础类型 + 扩展类型
struct TBasic {
    int8_t i8 = -1;
    int16_t i16 = -2;
    int32_t i32 = -3;
    int64_t i64 = -4;
    uint8_t ui8 = 1;
    uint16_t ui16 = 2;
    uint32_t ui32 = 3;
    uint64_t ui64 = 4;
    float f = 1.5f;
    double d = 2.5;
    bool b = true;
    std::string s = "Hi";
    xvorin::serdes::buffer buf = std::string("\0\1\2\3", 4);
    xvorin::serdes::envar env = "${HOME}/sth";
};

// 枚举(作普通成员/容器元素/map key)
enum class TColor { RED,
    GREEN,
    BLUE };

// 叶子对象
struct TLeaf {
    int x = 1;
    std::string y = "y";
};

// 嵌套对象(两层)
struct TNode {
    TLeaf leaf;
    int level = 0;
    std::string tag = "tag";
};

// 继承
struct TBase {
    int bi = 10;
    std::string bs = "bs";
};

struct TDerived : public TBase {
    std::string ds = "ds";
    TLeaf dl;
};

// STL 容器 + 嵌套 STL
struct TStl {
    std::vector<int> vi = { 1, 2, 3 };
    std::list<std::string> ls = { "a", "b" };
    std::map<std::string, int> mi = { { "one", 1 }, { "two", 2 } };
    std::unordered_map<int, std::string> umis = { { 1, "u1" }, { 2, "u2" } };
    std::set<int> si = { 5, 6 };
    std::unordered_set<std::string> uss = { "x", "y" };
    std::vector<std::vector<int>> vvi = { { 1, 2 }, { 3 } };
    std::map<std::string, std::vector<int>> mvi = { { "k", { 7, 8 } } };
    std::vector<TLeaf> vleaf = { { 1, "a" }, { 2, "b" } };
    std::map<std::string, TLeaf> mleaf = { { "L", { 9, "z" } } };
    std::map<TColor, std::string> mek = { { TColor::RED, "red" } };
};

// 智能指针
struct TPtr {
    std::shared_ptr<TLeaf> sp = std::make_shared<TLeaf>(TLeaf { 3, "sp" });
    std::unique_ptr<TLeaf> up = std::unique_ptr<TLeaf>(new TLeaf { 4, "up" });
    std::shared_ptr<TLeaf> nullsp = nullptr;
};

// subtree: 单元素 + 容器
struct TSubtreeRoot {
    int i = 1;
    xvorin::serdes::subtree<TLeaf> sub;
    std::vector<xvorin::serdes::subtree<TLeaf>> subs;
    std::map<std::string, xvorin::serdes::subtree<TLeaf>> submap;
};

// subtree 嵌套: 被引用的配置里再引用一份配置(main -> mid -> leaf)
struct TNestedLeaf {
    int a = 0;
};

struct TNestedMid {
    int m = 0;
    xvorin::serdes::subtree<TNestedLeaf> leaf;
};

struct TNestedRoot {
    int i = 0;
    xvorin::serdes::subtree<TNestedMid> mid;
};

// 带校验(verinfo)与变更回调的根
struct TValidated {
    int level = 5;
    std::string name = "abc";
};

// 命令行/文件 sink 用根
struct TFileRoot {
    int i = 1;
    std::string s = "s";
    std::vector<int> v = { 1, 2 };
};
