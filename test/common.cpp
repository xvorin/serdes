
#include "common.h"

DEFINE_PARAM(TestSubsubParam, subsub_char);
DEFINE_PARAM(TestSubsubParam, subsub_uchar);
DEFINE_PARAM(TestSubsubParam, subsub_int8);
DEFINE_PARAM(TestSubsubParam, subsub_uint8);
DEFINE_PARAM(TestSubsubParam, subsub_int32);
DEFINE_PARAM(TestSubsubParam, subsub_uint32);
DEFINE_PARAM(TestSubsubParam, subsub_int64);
DEFINE_PARAM(TestSubsubParam, subsub_uint64);
DEFINE_PARAM(TestSubsubParam, subsub_long);
DEFINE_PARAM(TestSubsubParam, subsub_ulong);
DEFINE_PARAM(TestSubsubParam, subsub_longlong);
DEFINE_PARAM(TestSubsubParam, subsub_ulonglong);
DEFINE_PARAM(TestSubsubParam, subsub_int);
DEFINE_PARAM(TestSubsubParam, subsub_double);
DEFINE_PARAM(TestSubsubParam, subsub_float);
DEFINE_PARAM(TestSubsubParam, subsub_string);

DEFINE_PARAM(TestSubParam, sub_char);
DEFINE_PARAM(TestSubParam, sub_uchar);
DEFINE_PARAM(TestSubParam, sub_int8);
DEFINE_PARAM(TestSubParam, sub_uint8);
DEFINE_PARAM(TestSubParam, sub_int32);
DEFINE_PARAM(TestSubParam, sub_uint32);
DEFINE_PARAM(TestSubParam, sub_int64);
DEFINE_PARAM(TestSubParam, sub_uint64);
DEFINE_PARAM(TestSubParam, sub_long);
DEFINE_PARAM(TestSubParam, sub_ulong);
DEFINE_PARAM(TestSubParam, sub_longlong);
DEFINE_PARAM(TestSubParam, sub_ulonglong);
DEFINE_PARAM(TestSubParam, sub_int);
DEFINE_PARAM(TestSubParam, sub_double);
DEFINE_PARAM(TestSubParam, sub_float);
DEFINE_PARAM(TestSubParam, sub_string);
DEFINE_PARAM(TestSubParam, sub_sub);

DEFINE_PARAM(TestParam, top_char);
DEFINE_PARAM(TestParam, top_uchar);
DEFINE_PARAM(TestParam, top_int8);
DEFINE_PARAM(TestParam, top_uint8);
DEFINE_PARAM(TestParam, top_int32);
DEFINE_PARAM(TestParam, top_uint32);
DEFINE_PARAM(TestParam, top_int64);
DEFINE_PARAM(TestParam, top_uint64);
DEFINE_PARAM(TestParam, top_long);
DEFINE_PARAM(TestParam, top_ulong);
DEFINE_PARAM(TestParam, top_longlong);
DEFINE_PARAM(TestParam, top_ulonglong);
DEFINE_PARAM(TestParam, top_int);
DEFINE_PARAM(TestParam, top_double);
DEFINE_PARAM(TestParam, top_float);
DEFINE_PARAM(TestParam, top_string);
DEFINE_PARAM(TestParam, top_sub);

DEFINE_PARAM(TestParam, top_vector_sub);
DEFINE_PARAM(TestParam, top_vector_vector_sub);
DEFINE_PARAM(TestParam, top_vector_vector_vector_sub);
DEFINE_PARAM(TestParam, top_vector_string);
DEFINE_PARAM(TestParam, top_vector_vector_int);
DEFINE_PARAM(TestParam, top_vector_vector_vector_string);

DEFINE_PARAM(TestParam, top_list_sub);
DEFINE_PARAM(TestParam, top_list_list_sub);
DEFINE_PARAM(TestParam, top_list_list_list_sub);
DEFINE_PARAM(TestParam, top_list_string);
DEFINE_PARAM(TestParam, top_list_list_int);
DEFINE_PARAM(TestParam, top_list_list_list_string);

DEFINE_PARAM(TestParam, top_map_sub);
DEFINE_PARAM(TestParam, top_map_map_sub);
DEFINE_PARAM(TestParam, top_map_map_map_sub);
DEFINE_PARAM(TestParam, top_map_string);
DEFINE_PARAM(TestParam, top_map_map_int);
DEFINE_PARAM(TestParam, top_map_map_map_string);
DEFINE_PARAM(TestParam, top_map_int64_string);

DEFINE_PARAM(TestParam, top_umap_sub);
DEFINE_PARAM(TestParam, top_umap_umap_sub);
DEFINE_PARAM(TestParam, top_umap_umap_umap_sub);
DEFINE_PARAM(TestParam, top_umap_string);
DEFINE_PARAM(TestParam, top_umap_umap_int);
DEFINE_PARAM(TestParam, top_umap_umap_umap_string);

DEFINE_PARAM(TestParam, top_set_string);
DEFINE_PARAM(TestParam, top_set_set_int);
DEFINE_PARAM(TestParam, top_set_set_set_string);

DEFINE_PARAM(TestParam, top_uset_string);
DEFINE_PARAM(TestParam, top_uset_int);

DEFINE_PARAM(TestParam, top_vector_map_umap_list_set_int);
DEFINE_PARAM(TestParam, top_umap_list_vector_map_sub);

std::shared_ptr<xvorin::serdes::WParameter<TestParam>> g_wp;

TestParam::TestParam()
{
    setup();
}
void TestParam::setup()
{
    top_char = 'a';
    top_uchar = 'b';
    top_int8 = 'c';
    top_uint8 = 'd';
    top_int32 = 5;
    top_uint32 = 6;
    top_int64 = 7;
    top_uint64 = 8;
    top_long = 9;
    top_ulong = 10;
    top_longlong = 11;
    top_ulonglong = 12;
    top_int = 13;
    top_double = 14.14;
    top_float = 15.15;
    top_string = "abc";
    top_sub = TestSubParam();

    //******************************************************************** vector
    top_vector_sub = { TestSubParam(), TestSubParam() };
    top_vector_vector_sub = { { TestSubParam(), TestSubParam() } };
    top_vector_vector_vector_sub = { { { TestSubParam() } } };

    top_vector_string = { "Hello1", "World2", "Hello3", "World4", "Hello5", "World6", "Hello7", "World8", "Hello9", "World10", "Hello11", "World12",
        "Hello13", "World14", "Hello15", "World16", "Hello17", "World18", "Hello19", "World20", "Hello21", "World22", "Hello23", "World24" };
    top_vector_vector_int = { { 4354, 453, 8907 }, { 123 } };
    top_vector_vector_vector_string = { { { "Hello", "World!" }, { "Hello World!!" } }, { { "Hello World!!!" } } };

    //******************************************************************** list
    top_list_sub = { TestSubParam(), TestSubParam() };
    top_list_sub.back().sub_int = 234;

    top_list_list_sub = { { TestSubParam(), TestSubParam() } };
    top_list_list_list_sub = { { { TestSubParam() } } };

    top_list_string = { "Hello", "World" };
    top_list_list_int = { { 4354, 453, 8907 }, { 123 } };
    top_list_list_list_string = { { { "Hello", "World!" }, { "Hello World!!" } }, { { "Hello World!!!" } } };

    //******************************************************************** map
    top_map_sub["map_sub0"] = TestSubParam();
    top_map_sub["map_sub1"] = TestSubParam();

    top_map_map_sub["map_map_sub0"]["map_map_sub00"] = TestSubParam();
    top_map_map_sub["map_map_sub1"]["map_map_sub10"] = TestSubParam();
    top_map_map_sub["map_map_sub1"]["map_map_sub11"] = TestSubParam();

    top_map_map_map_sub["map_map_map_sub0"]["map_map_map_sub00"]["map_map_map_sub000"] = TestSubParam();
    top_map_map_map_sub["map_map_map_sub1"]["map_map_map_sub10"]["map_map_map_sub100"] = TestSubParam();
    top_map_map_map_sub["map_map_map_sub1"]["map_map_map_sub11"]["map_map_map_sub110"] = TestSubParam();

    top_map_string["map_string0"] = "Hello";
    top_map_string["map_string1"] = "World";

    top_map_map_int["map_map_int0"]["map_map_int00"] = 4354;
    top_map_map_int["map_map_int0"]["map_map_int01"] = 453;
    top_map_map_int["map_map_int0"]["map_map_int02"] = 8907;
    top_map_map_int["map_map_int1"]["map_map_int10"] = 123;

    top_map_map_map_string["map_map_map_string0"]["map_map_map_string00"]["map_map_map_string000"] = "H";
    top_map_map_map_string["map_map_map_string0"]["map_map_map_string01"]["map_map_map_string010"] = "E";
    top_map_map_map_string["map_map_map_string0"]["map_map_map_string01"]["map_map_map_string012"] = "L";
    top_map_map_map_string["map_map_map_string1"]["map_map_map_string10"]["map_map_map_string100"] = "L";
    top_map_map_map_string["map_map_map_string1"]["map_map_map_string11"]["map_map_map_string110"] = "O";
    top_map_map_map_string["map_map_map_string1"]["map_map_map_string12"]["map_map_map_string120"] = "?";
    top_map_map_map_string["map_map_map_string2"]["map_map_map_string20"]["map_map_map_string200"] = "!";

    top_map_int64_string[1] = "a";
    top_map_int64_string[2] = "b";
    top_map_int64_string[3] = "c";

    //******************************************************************** unordered_map
    top_umap_sub["umap_sub0"] = TestSubParam();
    top_umap_sub["umap_sub1"] = TestSubParam();

    top_umap_umap_sub["umap_umap_sub0"]["umap_umap_sub00"] = TestSubParam();
    top_umap_umap_sub["umap_umap_sub1"]["umap_umap_sub10"] = TestSubParam();
    top_umap_umap_sub["umap_umap_sub1"]["umap_umap_sub11"] = TestSubParam();

    top_umap_umap_umap_sub["umap_umap_umap_sub0"]["umap_umap_umap_sub00"]["umap_umap_umap_sub000"] = TestSubParam();
    top_umap_umap_umap_sub["umap_umap_umap_sub1"]["umap_umap_umap_sub10"]["umap_umap_umap_sub100"] = TestSubParam();
    top_umap_umap_umap_sub["umap_umap_umap_sub1"]["umap_umap_umap_sub11"]["umap_umap_umap_sub110"] = TestSubParam();

    top_umap_string["umap_string0"] = "Hello";
    top_umap_string["umap_string1"] = "World";

    top_umap_umap_int["umap_umap_int0"]["umap_umap_int00"] = 4354;
    top_umap_umap_int["umap_umap_int0"]["umap_umap_int01"] = 453;
    top_umap_umap_int["umap_umap_int0"]["umap_umap_int02"] = 8907;
    top_umap_umap_int["umap_umap_int1"]["umap_umap_int10"] = 123;

    top_umap_umap_umap_string["umap_umap_umap_string0"]["umap_umap_umap_string00"]["umap_umap_umap_string000"] = "W";
    top_umap_umap_umap_string["umap_umap_umap_string0"]["umap_umap_umap_string01"]["umap_umap_umap_string010"] = "O";
    top_umap_umap_umap_string["umap_umap_umap_string0"]["umap_umap_umap_string01"]["umap_umap_umap_string012"] = "R";
    top_umap_umap_umap_string["umap_umap_umap_string1"]["umap_umap_umap_string10"]["umap_umap_umap_string100"] = "L";
    top_umap_umap_umap_string["umap_umap_umap_string1"]["umap_umap_umap_string11"]["umap_umap_umap_string110"] = "D";
    top_umap_umap_umap_string["umap_umap_umap_string1"]["umap_umap_umap_string12"]["umap_umap_umap_string120"] = "?";
    top_umap_umap_umap_string["umap_umap_umap_string2"]["umap_umap_umap_string20"]["umap_umap_umap_string200"] = "!";

    //******************************************************************** set
    top_set_string = { "Hello", "World" };
    top_set_set_int = { { 4354, 453, 8907 }, { 123 } };
    top_set_set_set_string = { { { "Hello", "World!" }, { "Hello World!!" } }, { { "Hello World!!!" } } };

    //******************************************************************** unordered_set
    top_uset_string = { "Hello", "World" };
    top_uset_int = { 4354, 453, 8907 };

    //******************************************************************** mixture
    top_vector_map_umap_list_set_int.emplace_back();
    top_vector_map_umap_list_set_int[0]["map"]["umap"].push_back({ 666, 666, 666, 668 });
    top_vector_map_umap_list_set_int[0]["map"]["umap"].push_back({ 8, 8, 8, 8, 8, 8, 8, 8 });

    top_vector_map_umap_list_set_int.emplace_back();
    top_vector_map_umap_list_set_int[1]["map"]["umap0"].push_back({ 6, 6, 5, 2, 342, 52, 234, 6 });
    top_vector_map_umap_list_set_int[1]["map"]["umap1"].push_back({ 1, 2, 3 });

    std::vector<std::map<std::string, TestSubParam>> vec0;
    vec0.emplace_back();
    vec0[0]["map"] = TestSubParam();
    vec0[0]["map"] = TestSubParam();
    top_umap_list_vector_map_sub["umap"].push_back(vec0);

    std::vector<std::map<std::string, TestSubParam>> vec1;
    vec1.emplace_back();
    vec1.emplace_back();
    vec1.emplace_back();
    vec1[0]["map0"] = TestSubParam();
    vec1[1]["map0"] = TestSubParam();
    vec1[1]["map1"] = TestSubParam();
    vec1[2]["map2"] = TestSubParam();
    top_umap_list_vector_map_sub["umap"].push_back(vec1);

    std::vector<std::map<std::string, TestSubParam>> vec2;
    vec2.emplace_back();
    vec2[0]["map"] = TestSubParam();
    top_umap_list_vector_map_sub["umap"].push_back(vec2);
}

void TestParam::clear()
{
    top_char = 0;
    top_uchar = 0;
    top_int8 = 0;
    top_uint8 = 0;
    top_int32 = 0;
    top_uint32 = 0;
    top_int64 = 0;
    top_uint64 = 0;
    top_long = 0;
    top_ulong = 0;
    top_longlong = 0;
    top_ulonglong = 0;
    top_int = 0;
    top_double = 0;
    top_float = 0;
    top_string.clear();
    top_sub.clear();

    //******************************************************************** vector
    top_vector_sub.clear();
    top_vector_vector_sub.clear();
    top_vector_vector_vector_sub.clear();

    top_vector_string.clear();
    top_vector_vector_int.clear();
    top_vector_vector_vector_string.clear();

    //******************************************************************** list
    top_list_sub.clear();
    top_list_list_sub.clear();
    top_list_list_list_sub.clear();

    top_list_string.clear();
    top_list_list_int.clear();
    top_list_list_list_string.clear();

    //******************************************************************** map
    top_map_sub.clear();
    top_map_map_sub.clear();
    top_map_map_map_sub.clear();

    top_map_string.clear();
    top_map_map_int.clear();
    top_map_map_map_string.clear();

    //******************************************************************** unordered_map
    top_umap_sub.clear();
    top_umap_umap_sub.clear();
    top_umap_umap_umap_sub.clear();

    top_umap_string.clear();
    top_umap_umap_int.clear();
    top_umap_umap_umap_string.clear();

    //******************************************************************** set
    top_set_string.clear();
    top_set_set_int.clear();
    top_set_set_set_string.clear();

    //******************************************************************** unordered_set
    top_uset_string.clear();
    top_uset_int.clear();

    //******************************************************************** mixture
    top_vector_map_umap_list_set_int.clear();
    top_umap_list_vector_map_sub.clear();
}

TestSubParam::TestSubParam()
{
    setup();
}

void TestSubParam::setup()
{
    sub_char = 'a';
    sub_uchar = 'b';
    sub_int8 = 'c';
    sub_uint8 = 'd';
    sub_int32 = 5;
    sub_uint32 = 6;
    sub_int64 = 7;
    sub_uint64 = 8;
    sub_long = 9;
    sub_ulong = 10;
    sub_longlong = 11;
    sub_ulonglong = 12;
    sub_int = 13;
    sub_double = 14.14;
    sub_float = 15.15;
    sub_string = "abc";
    sub_sub = TestSubsubParam();
}

void TestSubParam::clear()
{
    sub_char = 0;
    sub_uchar = 0;
    sub_int8 = 0;
    sub_uint8 = 0;
    sub_int32 = 0;
    sub_uint32 = 0;
    sub_int64 = 0;
    sub_uint64 = 0;
    sub_long = 0;
    sub_ulong = 0;
    sub_longlong = 0;
    sub_ulonglong = 0;
    sub_int = 0;
    sub_double = 0;
    sub_float = 0;
    sub_string.clear();
    sub_sub.clear();
}

TestSubsubParam::TestSubsubParam()
{
    setup();
}

void TestSubsubParam::setup()
{
    subsub_char = 'a';
    subsub_uchar = 'b';
    subsub_int8 = 'c';
    subsub_uint8 = 'd';
    subsub_int32 = 5;
    subsub_uint32 = 6;
    subsub_int64 = 7;
    subsub_uint64 = 8;
    subsub_long = 9;
    subsub_ulong = 10;
    subsub_longlong = 11;
    subsub_ulonglong = 12;
    subsub_int = 13;
    subsub_double = 14.14;
    subsub_float = 15.15;
    subsub_string = "abc";
}

void TestSubsubParam::clear()
{
    subsub_char = 0;
    subsub_uchar = 0;
    subsub_int8 = 0;
    subsub_uint8 = 0;
    subsub_int32 = 0;
    subsub_uint32 = 0;
    subsub_int64 = 0;
    subsub_uint64 = 0;
    subsub_long = 0;
    subsub_ulong = 0;
    subsub_longlong = 0;
    subsub_ulonglong = 0;
    subsub_int = 0;
    subsub_double = 0;
    subsub_float = 0;
    subsub_string.clear();
}
