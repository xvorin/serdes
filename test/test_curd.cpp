#include "common.h"

using namespace xvorin::serdes;

struct TestCreate {
    std::vector<std::string> vec = { "vec1", "vec2", "vec3" };
    std::vector<std::list<std::string>> veclist = { { "veclist1", "veclist2" }, { "veclist3" } };
    std::set<std::string> set = { "set1", "set2", "set3" };
    std::map<std::string, std::list<std::string>> map = { { "key1", { "value", "value2" } }, { "key2", { "value3" } } };
};

DEFINE_PARAM(TestCreate, vec)
DEFINE_PARAM(TestCreate, veclist)
DEFINE_PARAM(TestCreate, map)
DEFINE_PARAM(TestCreate, set)

TEST(PARAMETER, vector_curd)
{
    auto pw = xvorin::serdes::create<TestCreate>("root", "test_curd.json");

    /********************************************create******************************************/
    pw->create("root.vec.1");
    EXPECT_EQ(pw->get<std::string>("root.vec.0"), "vec1");
    EXPECT_EQ(pw->get<std::string>("root.vec.1"), "");
    EXPECT_EQ(pw->get<std::string>("root.vec.2"), "vec2");
    EXPECT_EQ(pw->get<std::string>("root.vec.3"), "vec3");

    pw->create("root.vec.5");
    pw->from_yaml("root.vec.4", "Hello World!");
    EXPECT_EQ(pw->get<std::string>("root.vec.0"), "vec1");
    EXPECT_EQ(pw->get<std::string>("root.vec.1"), "");
    EXPECT_EQ(pw->get<std::string>("root.vec.2"), "vec2");
    EXPECT_EQ(pw->get<std::string>("root.vec.3"), "vec3");
    EXPECT_EQ(pw->get<std::string>("root.vec.4"), "Hello World!");

    EXPECT_EQ(pw->parameter("root.vec")->children_size(), 5);

    /********************************************remove******************************************/
    try {
        pw->remove("root.vec.5");
    } catch (xvorin::serdes::Exception& e) {
        EXPECT_EQ(std::string(e.what()), std::string(xvorin::serdes::error_msg::PARAMETER_NOT_FOUND) + ":root.vec.5");
    }

    pw->remove("root.vec.0");
    EXPECT_EQ(pw->get<std::string>("root.vec.3"), "Hello World!");

    pw->remove("root.vec.0");
    EXPECT_EQ(pw->get<std::string>("root.vec.2"), "Hello World!");

    pw->remove("root.vec.0");
    EXPECT_EQ(pw->get<std::string>("root.vec.1"), "Hello World!");

    /********************************************get_set******************************************/
    EXPECT_EQ(pw->to_json("root.vec.1"), "\"Hello World!\"");
    EXPECT_EQ(pw->to_yaml("root.vec.1"), "Hello World!");

    EXPECT_EQ(pw->value().vec[1], "Hello World!");
}

TEST(PARAMETER, veclist_curd)
{
    auto pw = xvorin::serdes::create<TestCreate>("root", "test_curd.json");

    /********************************************create******************************************/
    pw->create("root.veclist.8");
    EXPECT_EQ(pw->parameter("root.veclist")->children_size(), 3);

    pw->create("root.veclist.2.1");
    EXPECT_EQ(pw->parameter("root.veclist.2")->children_size(), 1);

    try {
        pw->create("root.veclist.3.1");
    } catch (xvorin::serdes::Exception& e) {
        EXPECT_EQ(std::string(e.what()), std::string(xvorin::serdes::error_msg::PARAMETER_NOT_FOUND) + ":" + "root.veclist.3");
    }

    // pw->create("root.vec.5");
    // pw->from_yaml("root.vec.4", "Hello World!");
    // EXPECT_EQ(pw->get<std::string>("root.vec.0"), "vec1");
    // EXPECT_EQ(pw->get<std::string>("root.vec.1"), "");
    // EXPECT_EQ(pw->get<std::string>("root.vec.2"), "vec2");
    // EXPECT_EQ(pw->get<std::string>("root.vec.3"), "vec3");
    // EXPECT_EQ(pw->get<std::string>("root.vec.4"), "Hello World!");

    /********************************************remove******************************************/
    pw->remove("root.veclist.0.1");
    EXPECT_EQ(pw->parameter("root.veclist.0")->children_size(), 1);

    pw->remove("root.veclist.1.0");
    EXPECT_EQ(pw->parameter("root.veclist.1")->children_size(), 0);

    /********************************************get_set******************************************/
    EXPECT_EQ(pw->get<std::string>("root.veclist.0.0"), "veclist1");
}

TEST(PARAMETER, map_curd)
{
    auto pw = xvorin::serdes::create<TestCreate>("root", "test_curd.json");

    /********************************************create******************************************/
    try {
        pw->create("root.map.key1");
    } catch (std::exception& e) {
        EXPECT_EQ(std::string(e.what()), std::string(error_msg::INDEX_DUMPLICATE) + ":root.map.key1");
    }

    pw->create("root.map.key3");
    EXPECT_EQ(pw->parameter("root.map.key3")->children_size(), 0);

    /********************************************remove******************************************/
    try {
        pw->create("root.map.key2");
    } catch (std::exception& e) {
        EXPECT_EQ(std::string(e.what()), std::string(error_msg::INDEX_DUMPLICATE) + ":root.map.key2");
    }

    try {
        pw->remove("root.map.key8");
    } catch (std::exception& e) {
        EXPECT_EQ(std::string(e.what()), std::string(error_msg::PARAMETER_NOT_FOUND) + ":root.map.key8");
    }

    pw->remove("root.map.key2");
    EXPECT_EQ(pw->parameter("root.map")->children_size(), 2);
}

TEST(PARAMETER, set_curd)
{
    auto pw = xvorin::serdes::create<TestCreate>("root", "test_curd.json");

    /********************************************create******************************************/
    pw->create("root.set.1");
    EXPECT_EQ(pw->get<std::string>("root.set.1"), "");

    pw->create("root.set.5");
    pw->from_yaml("root.set.4", "Hello World!");

    EXPECT_EQ(pw->parameter("root.set")->children_size(), 5);

    /********************************************remove******************************************/
    try {
        pw->remove("root.set.5");
    } catch (xvorin::serdes::Exception& e) {
        EXPECT_EQ(e.what(), std::string(xvorin::serdes::error_msg::PARAMETER_NOT_FOUND) + ":root.set.5");
    }

    pw->remove("root.set.0");
    EXPECT_EQ(pw->parameter("root.set")->children_size(), 4);
    pw->remove("root.set.0");
    EXPECT_EQ(pw->parameter("root.set")->children_size(), 3);
    pw->remove("root.set.0");
    EXPECT_EQ(pw->parameter("root.set")->children_size(), 2);

    EXPECT_NE(pw->value().set.find("Hello World!"), pw->value().set.end());
}