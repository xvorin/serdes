#include "common.h"

using namespace xvorin::serdes;

#define UNORDERED_SET_BCHECK(s)                                \
    {                                                          \
        using T = typename decltype(s)::value_type;            \
        std::set<T> s1, s2;                                    \
        size_t i = 0;                                          \
        for (auto& item : s) {                                 \
            const auto key = std::string(#s)                   \
                                 .append(".")                  \
                                 .append(std::to_string(i++)); \
            s1.insert(item);                                   \
            s2.insert(g_wp->parameter<T>(key)->value);         \
        }                                                      \
        ASSERT_EQ(s1, s2);                                     \
    }

TEST(SET_TYPE, deserialize)
{
    auto root = g_wp->value();

    //******************************************************************** set
    // std::set<std::string> top_set_string;
    CHECK_STL_SIZE(root.top_set_string);
    UNORDERED_SET_BCHECK(root.top_set_string);

    // std::set<std::set<int>> top_set_set_int;
    CHECK_STL_SIZE(root.top_set_set_int);
    // CHECK_STL_SIZE2(root.top_set_set_int);
    // CHECK_STL_BASIC_VALUE2(root.top_set_set_int);

    // std::set<std::set<std::set<std::string>>> top_set_set_set_string;
    CHECK_STL_SIZE(root.top_set_set_set_string);
    // CHECK_STL_SIZE2(root.top_set_set_set_string);
    // CHECK_STL_SIZE3(root.top_set_set_set_string);
    // CHECK_STL_BASIC_VALUE3(root.top_set_set_set_string);

    //******************************************************************** unordered_set
    // std::unordered_set<std::string> top_uset_string;
    CHECK_STL_SIZE(root.top_uset_string);
    UNORDERED_SET_BCHECK(root.top_uset_string);

    // std::unordered_set<int> top_uset_int;
    CHECK_STL_SIZE(root.top_uset_int);
    UNORDERED_SET_BCHECK(root.top_uset_int);
}

TEST(SET_TYPE, serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    //******************************************************************** set
    // std::set<std::string> top_set_string;
    CHECK_STL_SIZE(root.top_set_string);
    UNORDERED_SET_BCHECK(root.top_set_string);

    // std::set<std::set<int>> top_set_set_int;
    CHECK_STL_SIZE(root.top_set_set_int);
    // CHECK_STL_SIZE2(root.top_set_set_int);
    // CHECK_STL_BASIC_VALUE2(root.top_set_set_int);

    // std::set<std::set<std::set<std::string>>> top_set_set_set_string;
    CHECK_STL_SIZE(root.top_set_set_set_string);
    // CHECK_STL_SIZE2(root.top_set_set_set_string);
    // CHECK_STL_SIZE3(root.top_set_set_set_string);
    // CHECK_STL_BASIC_VALUE3(root.top_set_set_set_string);

    //******************************************************************** unordered_set
    // std::unordered_set<std::string> top_uset_string;
    CHECK_STL_SIZE(root.top_uset_string);
    UNORDERED_SET_BCHECK(root.top_uset_string);

    // std::unordered_set<int> top_uset_int;
    CHECK_STL_SIZE(root.top_uset_int);
    UNORDERED_SET_BCHECK(root.top_uset_int);
}
