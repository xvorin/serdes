#include "common.h"

using namespace xvorin::serdes;

template <typename T>
void show_traits_result()
{
    auto key = Parameter::readable_detail_type(typeid(T));

    std::string space = std::string(50 - key.size(), ' ');
    std::cout << Parameter::readable_detail_type(typeid(T)) << space << ":"
              << " basic " << xvorin::serdes::is_basic<T>::value
              << " vector " << xvorin::serdes::is_vector<T>::value
              << " list " << xvorin::serdes::is_list<T>::value
              << " sequence " << xvorin::serdes::is_sequence<T>::value
              << " rbmap " << xvorin::serdes::is_rbmap<T>::value
              << " hashmap " << xvorin::serdes::is_hashmap<T>::value
              << " map " << xvorin::serdes::is_map<T>::value
              << " rbset " << xvorin::serdes::is_rbset<T>::value
              << " hashset " << xvorin::serdes::is_hashset<T>::value
              << " set " << xvorin::serdes::is_set<T>::value
              << " object " << xvorin::serdes::is_object<T>::value
              << std::endl;
}

template <typename T>
void test_traits_result(std::vector<bool> bs)
{
    auto key = Parameter::readable_detail_type(typeid(T));
    std::vector<bool> result = {
        xvorin::serdes::is_enum<T>::value,
        xvorin::serdes::is_basic<T>::value,
        xvorin::serdes::is_vector<T>::value,
        xvorin::serdes::is_list<T>::value,
        xvorin::serdes::is_sequence<T>::value,
        xvorin::serdes::is_rbmap<T>::value,
        xvorin::serdes::is_hashmap<T>::value,
        xvorin::serdes::is_map<T>::value,
        xvorin::serdes::is_rbset<T>::value,
        xvorin::serdes::is_hashset<T>::value,
        xvorin::serdes::is_set<T>::value,
        xvorin::serdes::is_object<T>::value
    };

    ASSERT_EQ(bs, result);
}

template <typename T>
void test_traits(std::vector<bool> bs)
{
    // show_traits_result<T>();
    test_traits_result<T>(bs);
    // show_traits_multi_result<T&>();
    // show_traits_multi_result<T*>();
    // show_traits_multi_result<const T>();
    // show_traits_multi_result<const T&>();
    // show_traits_multi_result<const T*>();
}

enum class TestEnum {
};

#include <queue>

TEST(TRAITS, show)
{
    test_traits<int>({ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<double>({ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<std::string>({ 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<TestEnum>({ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<TestParam>({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 });

    test_traits<std::vector<int>>({ 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<std::list<int>>({ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<std::map<int, int>>({ 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 });
    test_traits<std::set<int>>({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0 });
    test_traits<std::unordered_map<int, int>>({ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 });
    test_traits<std::unordered_set<int>>({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 });

    test_traits<std::vector<std::list<int>>>({ 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<std::list<std::vector<int>>>({ 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 });
    test_traits<std::map<int, std::list<std::vector<int>>>>({ 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 });
    test_traits<std::set<std::unordered_map<int, int>>>({ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0 });
    test_traits<std::unordered_map<int, int>>({ 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 });
    test_traits<std::unordered_set<std::string>>({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 });
}