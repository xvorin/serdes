#include "common.h"

using namespace xvorin::serdes;

TEST(VECTOR_TYPE, deserialize)
{
    auto root = g_wp->value();

    // std::vector<TestSubParam> top_vector_sub;
    CHECK_STL_SIZE(root.top_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_string);

    // std::vector<std::vector<TestSubParam>> top_vector_vector_sub;
    CHECK_STL_SIZE(root.top_vector_vector_sub);
    CHECK_STL_SIZE2(root.top_vector_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_string);

    // std::vector<std::vector<std::vector<TestSubParam>>> top_vector_vector_vector_sub;
    CHECK_STL_SIZE(root.top_vector_vector_vector_sub);
    CHECK_STL_SIZE2(root.top_vector_vector_vector_sub);
    CHECK_STL_SIZE3(root.top_vector_vector_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_string);

    // std::vector<std::string> top_vector_string;
    CHECK_STL_SIZE(root.top_vector_string);
    CHECK_STL_BASIC_VALUE(root.top_vector_string);

    // std::vector<std::vector<int>> top_vector_vector_int;
    CHECK_STL_SIZE(root.top_vector_vector_int);
    CHECK_STL_SIZE2(root.top_vector_vector_int);
    CHECK_STL_BASIC_VALUE2(root.top_vector_vector_int);

    // std::vector<std::vector<std::vector<std::string>>> top_vector_vector_vector_string;
    CHECK_STL_SIZE(root.top_vector_vector_vector_string);
    CHECK_STL_SIZE2(root.top_vector_vector_vector_string);
    CHECK_STL_SIZE3(root.top_vector_vector_vector_string);
    CHECK_STL_BASIC_VALUE3(root.top_vector_vector_vector_string);
}

TEST(VECTOR_TYPE, serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    // std::vector<TestSubParam> top_vector_sub;
    CHECK_STL_SIZE(root.top_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE(root.top_vector_sub, sub_string);

    // std::vector<std::vector<TestSubParam>> top_vector_vector_sub;
    CHECK_STL_SIZE(root.top_vector_vector_sub);
    CHECK_STL_SIZE2(root.top_vector_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE2(root.top_vector_vector_sub, sub_string);

    // std::vector<std::vector<std::vector<TestSubParam>>> top_vector_vector_vector_sub;
    CHECK_STL_SIZE(root.top_vector_vector_vector_sub);
    CHECK_STL_SIZE2(root.top_vector_vector_vector_sub);
    CHECK_STL_SIZE3(root.top_vector_vector_vector_sub);

    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uchar);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int8);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint8);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int32);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint32);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int64);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_uint64);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_long);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_ulong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_longlong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_ulonglong);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_int);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_double);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_float);
    CHECK_STL_STRUCT_BASIC_VALUE3(root.top_vector_vector_vector_sub, sub_string);

    // std::vector<std::string> top_vector_string;
    CHECK_STL_SIZE(root.top_vector_string);
    CHECK_STL_BASIC_VALUE(root.top_vector_string);

    // std::vector<std::vector<int>> top_vector_vector_int;
    CHECK_STL_SIZE(root.top_vector_vector_int);
    CHECK_STL_SIZE2(root.top_vector_vector_int);
    CHECK_STL_BASIC_VALUE2(root.top_vector_vector_int);

    // std::vector<std::vector<std::vector<std::string>>> top_vector_vector_vector_string;
    CHECK_STL_SIZE(root.top_vector_vector_vector_string);
    CHECK_STL_SIZE2(root.top_vector_vector_vector_string);
    CHECK_STL_SIZE3(root.top_vector_vector_vector_string);
    CHECK_STL_BASIC_VALUE3(root.top_vector_vector_vector_string);
}
