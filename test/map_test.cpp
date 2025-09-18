#include "common.h"

using namespace xvorin::serdes;

TEST(MAP_TYPE, deserialize)
{
    auto root = g_wp->value();

    // std::map<std::string, TestSubParam> top_map_sub;
    CHECK_MAP_SIZE(root.top_map_sub);

    // std::map<std::string, std::map<std::string, TestSubParam>> top_map_map_sub;
    CHECK_MAP_SIZE(root.top_map_map_sub);
    CHECK_MAP_SIZE2(root.top_map_map_sub);

    // std::map<std::string, std::map<std::string, std::map<std::string, TestSubParam>>> top_map_map_map_sub;
    CHECK_MAP_SIZE(root.top_map_map_map_sub);
    CHECK_MAP_SIZE2(root.top_map_map_map_sub);
    CHECK_MAP_SIZE3(root.top_map_map_map_sub);

    // std::map<std::string, std::string> top_map_string;
    CHECK_MAP_SIZE(root.top_map_string);
    CHECK_MAP_BASIC_VALUE(root.top_map_string);

    // std::map<std::string, std::map<std::string, int>> top_map_map_int;
    CHECK_MAP_SIZE(root.top_map_map_int);
    CHECK_MAP_SIZE2(root.top_map_map_int);
    CHECK_MAP_BASIC_VALUE2(root.top_map_map_int);

    // std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> top_map_map_map_string;
    CHECK_MAP_SIZE(root.top_map_map_map_string);
    CHECK_MAP_SIZE2(root.top_map_map_map_string);
    CHECK_MAP_SIZE3(root.top_map_map_map_string);
    CHECK_MAP_BASIC_VALUE3(root.top_map_map_map_string);

    // //******************************************************************** unordered_map
    // std::unordered_map<std::string, TestSubParam> top_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_sub);

    // std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>> top_umap_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_umap_sub);
    CHECK_MAP_SIZE2(root.top_umap_umap_sub);

    // std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>>> top_umap_umap_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_umap_umap_sub);
    CHECK_MAP_SIZE2(root.top_umap_umap_umap_sub);
    CHECK_MAP_SIZE3(root.top_umap_umap_umap_sub);

    // std::unordered_map<std::string, std::string> top_umap_string;
    CHECK_MAP_SIZE(root.top_umap_string);
    CHECK_MAP_BASIC_VALUE(root.top_umap_string);

    // std::unordered_map<std::string, std::unordered_map<std::string, int>> top_umap_umap_int;
    CHECK_MAP_SIZE(root.top_umap_umap_int);
    CHECK_MAP_SIZE2(root.top_umap_umap_int);
    CHECK_MAP_BASIC_VALUE2(root.top_umap_umap_int);

    // std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>> top_umap_umap_umap_string;
    CHECK_MAP_SIZE(root.top_umap_umap_umap_string);
    CHECK_MAP_SIZE2(root.top_umap_umap_umap_string);
    CHECK_MAP_SIZE3(root.top_umap_umap_umap_string);
    CHECK_MAP_BASIC_VALUE3(root.top_umap_umap_umap_string);
}

TEST(MAP_TYPE, serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    // std::map<std::string, TestSubParam> top_map_sub;
    CHECK_MAP_SIZE(root.top_map_sub);

    // std::map<std::string, std::map<std::string, TestSubParam>> top_map_map_sub;
    CHECK_MAP_SIZE(root.top_map_map_sub);
    CHECK_MAP_SIZE2(root.top_map_map_sub);

    // std::map<std::string, std::map<std::string, std::map<std::string, TestSubParam>>> top_map_map_map_sub;
    CHECK_MAP_SIZE(root.top_map_map_map_sub);
    CHECK_MAP_SIZE2(root.top_map_map_map_sub);
    CHECK_MAP_SIZE3(root.top_map_map_map_sub);

    // std::map<std::string, std::string> top_map_string;
    CHECK_MAP_SIZE(root.top_map_string);
    CHECK_MAP_BASIC_VALUE(root.top_map_string);

    // std::map<std::string, std::map<std::string, int>> top_map_map_int;
    CHECK_MAP_SIZE(root.top_map_map_int);
    CHECK_MAP_SIZE2(root.top_map_map_int);
    CHECK_MAP_BASIC_VALUE2(root.top_map_map_int);

    // std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> top_map_map_map_string;
    CHECK_MAP_SIZE(root.top_map_map_map_string);
    CHECK_MAP_SIZE2(root.top_map_map_map_string);
    CHECK_MAP_SIZE3(root.top_map_map_map_string);
    CHECK_MAP_BASIC_VALUE3(root.top_map_map_map_string);

    // //******************************************************************** unordered_map
    // std::unordered_map<std::string, TestSubParam> top_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_sub);

    // std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>> top_umap_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_umap_sub);
    CHECK_MAP_SIZE2(root.top_umap_umap_sub);

    // std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>>> top_umap_umap_umap_sub;
    CHECK_MAP_SIZE(root.top_umap_umap_umap_sub);
    CHECK_MAP_SIZE2(root.top_umap_umap_umap_sub);
    CHECK_MAP_SIZE3(root.top_umap_umap_umap_sub);

    // std::unordered_map<std::string, std::string> top_umap_string;
    CHECK_MAP_SIZE(root.top_umap_string);
    CHECK_MAP_BASIC_VALUE(root.top_umap_string);

    // std::unordered_map<std::string, std::unordered_map<std::string, int>> top_umap_umap_int;
    CHECK_MAP_SIZE(root.top_umap_umap_int);
    CHECK_MAP_SIZE2(root.top_umap_umap_int);
    CHECK_MAP_BASIC_VALUE2(root.top_umap_umap_int);

    // std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>> top_umap_umap_umap_string;
    CHECK_MAP_SIZE(root.top_umap_umap_umap_string);
    CHECK_MAP_SIZE2(root.top_umap_umap_umap_string);
    CHECK_MAP_SIZE3(root.top_umap_umap_umap_string);
    CHECK_MAP_BASIC_VALUE3(root.top_umap_umap_umap_string);
}
