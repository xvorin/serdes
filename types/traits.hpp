#pragma once

#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <type_traits>

#include "serdes/types/extension/buffer.hpp"
#include "serdes/types/extension/envar.hpp"

namespace xvorin::serdes {

template <typename T>
struct is_extension_basic : std::integral_constant<bool, std::is_same<T, buffer>::value || std::is_same<T, envar>::value> {
};

template <typename T>
struct is_basic : std::integral_constant<bool, std::is_arithmetic<T>::value || std::is_same<T, std::string>::value || is_extension_basic<T>::value> {
};

template <typename T>
struct is_enum : std::integral_constant<bool, std::is_enum<T>::value> {
};

// 判断是否为 shared_ptr
template <typename T>
struct is_shared_ptr : std::false_type { };

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type { };

// 判断是否为 unique_ptr
template <typename T>
struct is_unique_ptr : std::false_type { };

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type { };

// 判断是否为智能指针（shared_ptr 或 unique_ptr）
template <typename T>
struct is_smart_ptr : std::integral_constant<bool, is_shared_ptr<T>::value || is_unique_ptr<T>::value> { };

template <typename T, typename E = void>
struct is_vector : std::false_type {
};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type {
};

template <typename T, typename E = void>
struct is_list : std::false_type {
};

template <typename T>
struct is_list<std::list<T>> : std::true_type {
};

template <typename T>
struct is_sequence : std::integral_constant<bool, is_list<T>::value || is_vector<T>::value> {
};

template <typename T>
struct is_rbmap : std::false_type {
};

template <typename K, typename V>
struct is_rbmap<std::map<K, V>> : std::true_type {
};

template <typename T, typename E = void>
struct is_hashmap : std::false_type {
};

template <typename K, typename V>
struct is_hashmap<std::unordered_map<K, V>> : std::true_type {
};

template <typename T>
struct is_map : std::integral_constant<bool, is_rbmap<T>::value || is_hashmap<T>::value> {
};

template <typename T>
struct is_rbset : std::false_type {
};

template <typename T>
struct is_rbset<std::set<T>> : std::true_type {
};

template <typename T>
struct is_hashset : std::false_type {
};

template <typename T>
struct is_hashset<std::unordered_set<T>> : std::true_type {
};

template <typename T>
struct is_set : std::integral_constant<bool, is_rbset<T>::value || is_hashset<T>::value> {
};

template <typename T>
struct is_object : std::integral_constant<bool, std::is_class<T>::value && !is_basic<T>::value && !is_sequence<T>::value && !is_map<T>::value && !is_set<T>::value && !is_smart_ptr<T>::value> {
};

template <typename T>
struct is_stl : std::integral_constant<bool, is_sequence<T>::value || is_map<T>::value || is_set<T>::value || is_smart_ptr<T>::value> {
};

template <typename T>
struct is_composite : std::integral_constant<bool, (is_stl<T>::value && !is_smart_ptr<T>::value) || is_object<T>::value> {
};

template <typename T>
struct is_support : std::integral_constant<bool, !std::is_pointer<T>::value && !std::is_reference<T>::value && !std::is_const<T>::value && !std::is_volatile<T>::value> {
};

} // namespace xvorin::serdes