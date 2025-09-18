
#pragma once

#include "serdes/param.h"

#include <gtest/gtest.h>

struct TestSubsubParam {
    TestSubsubParam();
    void setup();
    void clear();

    char subsub_char;
    u_char subsub_uchar;
    int8_t subsub_int8;
    uint8_t subsub_uint8;
    int32_t subsub_int32;
    uint32_t subsub_uint32;
    int64_t subsub_int64;
    uint64_t subsub_uint64;
    long subsub_long;
    unsigned long subsub_ulong;
    long long subsub_longlong;
    unsigned long long subsub_ulonglong;
    int subsub_int;
    double subsub_double;
    float subsub_float;
    std::string subsub_string;
};

struct TestSubParam {
    TestSubParam();
    void setup();
    void clear();

    char sub_char;
    u_char sub_uchar;
    int8_t sub_int8;
    uint8_t sub_uint8;
    int32_t sub_int32;
    uint32_t sub_uint32;
    int64_t sub_int64;
    uint64_t sub_uint64;
    long sub_long;
    unsigned long sub_ulong;
    long long sub_longlong;
    unsigned long long sub_ulonglong;
    int sub_int;
    double sub_double;
    float sub_float;
    std::string sub_string;
    TestSubsubParam sub_sub;
};

struct TestParam {
    TestParam();
    void setup();
    void clear();

    char top_char;
    u_char top_uchar;
    int8_t top_int8;
    uint8_t top_uint8;
    int32_t top_int32;
    uint32_t top_uint32;
    int64_t top_int64;
    uint64_t top_uint64;
    long top_long;
    unsigned long top_ulong;
    long long top_longlong;
    unsigned long long top_ulonglong;
    int top_int;
    double top_double;
    float top_float;
    std::string top_string;
    TestSubParam top_sub;

    //******************************************************************** vector
    std::vector<TestSubParam> top_vector_sub;
    std::vector<std::vector<TestSubParam>> top_vector_vector_sub;
    std::vector<std::vector<std::vector<TestSubParam>>> top_vector_vector_vector_sub;

    std::vector<std::string> top_vector_string;
    std::vector<std::vector<int>> top_vector_vector_int;
    std::vector<std::vector<std::vector<std::string>>> top_vector_vector_vector_string;
    //******************************************************************** list
    std::list<TestSubParam> top_list_sub;
    std::list<std::list<TestSubParam>> top_list_list_sub;
    std::list<std::list<std::list<TestSubParam>>> top_list_list_list_sub;

    std::list<std::string> top_list_string;
    std::list<std::list<int>> top_list_list_int;
    std::list<std::list<std::list<std::string>>> top_list_list_list_string;

    //******************************************************************** map
    std::map<std::string, TestSubParam> top_map_sub;
    std::map<std::string, std::map<std::string, TestSubParam>> top_map_map_sub;
    std::map<std::string, std::map<std::string, std::map<std::string, TestSubParam>>> top_map_map_map_sub;

    std::map<std::string, std::string> top_map_string;
    std::map<std::string, std::map<std::string, int>> top_map_map_int;
    std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> top_map_map_map_string;

    std::map<int64_t, std::string> top_map_int64_string;

    //******************************************************************** unordered_map
    std::unordered_map<std::string, TestSubParam> top_umap_sub;
    std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>> top_umap_umap_sub;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, TestSubParam>>> top_umap_umap_umap_sub;

    std::unordered_map<std::string, std::string> top_umap_string;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> top_umap_umap_int;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>> top_umap_umap_umap_string;

    //******************************************************************** set
    std::set<std::string> top_set_string;
    std::set<std::set<int>> top_set_set_int;
    std::set<std::set<std::set<std::string>>> top_set_set_set_string;

    //******************************************************************** unordered_set
    std::unordered_set<std::string> top_uset_string;
    std::unordered_set<int> top_uset_int;

    //******************************************************************** mixture
    std::vector<std::map<std::string, std::unordered_map<std::string, std::list<std::set<int>>>>> top_vector_map_umap_list_set_int;
    std::unordered_map<std::string, std::list<std::vector<std::map<std::string, TestSubParam>>>> top_umap_list_vector_map_sub;
};

extern std::shared_ptr<xvorin::serdes::WParameter<TestParam>> g_wp;

#include "macro.h"