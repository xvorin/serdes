#include "definition.h"
#include "serdes/serdes.h"

// 注册成员
DEFINE_PARAM(BaseParam, v1, "测试commont1", nullptr, "[1,5);(4,3)");
DEFINE_PARAM(BaseParam, v2, "测试commont2", nullptr, "(0,5.8]");
DEFINE_PARAM(SubParam, v3, "测试commont3", nullptr, "^\\d{3}$");

DEFINE_PARAM(Param, x, "整型变量", nullptr);
DEFINE_PARAM(Param, y, "电话号码", nullptr, "^\\d{3}-\\d{8}$");
DEFINE_PARAM(Param, z, "布尔变量");
DEFINE_PARAM(Param, h, "浮点变量");
DEFINE_PARAM(Param, e, "enum变量");

DEFINE_PARAM(Param, o);
DEFINE_PARAM(Param, n);
DEFINE_PARAM(Param, m);
DEFINE_PARAM(Param, p);
DEFINE_PARAM(Param, q);
DEFINE_PARAM(Param, r);
// 注册继承关系
DEFINE_INHERIT(BaseParam, SubParam);

// 注册根节点(可选)
DEFINE_ROOT(Param);
