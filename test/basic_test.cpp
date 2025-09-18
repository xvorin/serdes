#include "common.h"

using namespace xvorin::serdes;

TEST(BASIC_TYPE, deserialize)
{
    auto root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_string);
}

TEST(BASIC_TYPE, object_sub_deserialize)
{
    auto root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_string);
}

TEST(BASIC_TYPE, object_subsub_deserialize)
{
    auto root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_string);
}

TEST(BASIC_TYPE, serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_string);
}

TEST(BASIC_TYPE, object_sub_serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_string);
}

TEST(BASIC_TYPE, object_sub_sub_serialize)
{
    TestParam root;
    root.clear();
    root = g_wp->value();

    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_char);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uchar);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint8);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint32);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_uint64);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_long);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_ulong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_longlong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_ulonglong);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_int);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_double);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_float);
    CHECK_STRUCT_BASIC_VALUE(root.top_sub.sub_sub.subsub_string);
}