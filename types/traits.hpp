#pragma once

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <type_traits>

namespace xvorin::serdes {

template <typename T>
struct is_basic : std::integral_constant<bool, std::is_arithmetic<T>::value || std::is_same<T, std::string>::value> {
};

template <typename T>
struct is_enum : std::integral_constant<bool, std::is_enum<T>::value> {
};

template <typename T, typename E = void>
struct is_vector : std::false_type {
};

template <typename T>
struct is_vector<T, typename std::enable_if<std::is_same<T, std::vector<typename T::value_type>>::value>::type> : std::true_type {
};

template <typename T, typename E = void>
struct is_list : std::false_type {
};

template <typename T>
struct is_list<T, typename std::enable_if<std::is_same<T, std::list<typename T::value_type>>::value>::type> : std::true_type {
};

template <typename T>
struct is_sequence : std::integral_constant<bool, is_list<T>::value || is_vector<T>::value> {
};

template <typename T, typename E = void>
struct is_rbmap : std::false_type {
};

template <typename T>
struct is_rbmap<T, typename std::enable_if<std::is_same<T, std::map<typename T::key_type, typename T::mapped_type>>::value>::type> : std::true_type {
};

template <typename T, typename E = void>
struct is_hashmap : std::false_type {
};

template <typename T>
struct is_hashmap<T, typename std::enable_if<std::is_same<T, std::unordered_map<typename T::key_type, typename T::mapped_type>>::value>::type> : std::true_type {
};

template <typename T>
struct is_map : std::integral_constant<bool, is_rbmap<T>::value || is_hashmap<T>::value> {
};

template <typename T, typename E = void>
struct is_rbset : std::false_type {
};

template <typename T>
struct is_rbset<T, typename std::enable_if<std::is_same<T, std::set<typename T::value_type>>::value>::type> : std::true_type {
};

template <typename T, typename E = void>
struct is_hashset : std::false_type {
};

template <typename T>
struct is_hashset<T, typename std::enable_if<std::is_same<T, std::unordered_set<typename T::value_type>>::value>::type> : std::true_type {
};

template <typename T>
struct is_set : std::integral_constant<bool, is_rbset<T>::value || is_hashset<T>::value> {
};

template <typename T>
struct is_object : std::integral_constant<bool, std::is_class<T>::value && !is_basic<T>::value && !is_sequence<T>::value && !is_map<T>::value && !is_set<T>::value> {
};

template <typename T>
struct is_stl : std::integral_constant<bool, is_sequence<T>::value || is_map<T>::value || is_set<T>::value> {
};

template <typename T>
struct is_composite : std::integral_constant<bool, is_stl<T>::value || is_object<T>::value> {
};

}