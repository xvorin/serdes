#include "definitions.hpp"

// 全部注册集中在独立 TU, 验证"注册与使用分离"(跨TU) 场景

DEFINE_PARAM(TBasic, i8, "i8")
DEFINE_PARAM(TBasic, i16, "i16")
DEFINE_PARAM(TBasic, i32, "i32")
DEFINE_PARAM(TBasic, i64, "i64")
DEFINE_PARAM(TBasic, ui8, "ui8")
DEFINE_PARAM(TBasic, ui16, "ui16")
DEFINE_PARAM(TBasic, ui32, "ui32")
DEFINE_PARAM(TBasic, ui64, "ui64")
DEFINE_PARAM(TBasic, f, "f")
DEFINE_PARAM(TBasic, d, "d")
DEFINE_PARAM(TBasic, b, "b")
DEFINE_PARAM(TBasic, s, "s")
DEFINE_PARAM(TBasic, buf, "buf")
DEFINE_PARAM(TBasic, env, "env")

DEFINE_ENUM(TColor, RED)
DEFINE_ENUM(TColor, GREEN)
DEFINE_ENUM(TColor, BLUE)

DEFINE_PARAM(TLeaf, x, "x")
DEFINE_PARAM(TLeaf, y, "y")

DEFINE_PARAM(TNode, leaf, "leaf")
DEFINE_PARAM(TNode, level, "level")
DEFINE_PARAM(TNode, tag, "tag")

DEFINE_PARAM(TBase, bi, "bi")
DEFINE_PARAM(TBase, bs, "bs")
DEFINE_INHERIT(TBase, TDerived)
DEFINE_PARAM(TDerived, ds, "ds")
DEFINE_PARAM(TDerived, dl, "dl")

DEFINE_PARAM(TStl, vi, "vi")
DEFINE_PARAM(TStl, ls, "ls")
DEFINE_PARAM(TStl, mi, "mi")
DEFINE_PARAM(TStl, umis, "umis")
DEFINE_PARAM(TStl, si, "si")
DEFINE_PARAM(TStl, uss, "uss")
DEFINE_PARAM(TStl, vvi, "vvi")
DEFINE_PARAM(TStl, mvi, "mvi")
DEFINE_PARAM(TStl, vleaf, "vleaf")
DEFINE_PARAM(TStl, mleaf, "mleaf")
DEFINE_PARAM(TStl, mek, "mek")

DEFINE_PARAM(TPtr, sp, "sp")
DEFINE_PARAM(TPtr, up, "up")
DEFINE_PARAM(TPtr, nullsp, "nullsp")

DEFINE_PARAM(TSubtreeRoot, i, "i")
DEFINE_PARAM(TSubtreeRoot, sub, "sub")
DEFINE_PARAM(TSubtreeRoot, subs, "subs")
DEFINE_PARAM(TSubtreeRoot, submap, "submap")

// subtree 嵌套(main -> mid -> leaf)
DEFINE_PARAM(TNestedLeaf, a, "a")
DEFINE_PARAM(TNestedMid, m, "m")
DEFINE_PARAM(TNestedMid, leaf, "leaf")
DEFINE_PARAM(TNestedRoot, i, "i")
DEFINE_PARAM(TNestedRoot, mid, "mid")

DEFINE_PARAM(TValidated, level, "level", nullptr, "(1,6)")
DEFINE_PARAM(TValidated, name, "name", nullptr, "^[a-z]+$")

DEFINE_PARAM(TFileRoot, i, "i")
DEFINE_PARAM(TFileRoot, s, "s")
DEFINE_PARAM(TFileRoot, v, "v")
