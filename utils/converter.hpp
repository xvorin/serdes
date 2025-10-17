#pragma once

#include "serdes/utils/base64.hpp"
#include "serdes/utils/utils.hpp"

#include "serdes/types/prototype.hpp"

#include <sstream>
#include <string>
#include <type_traits>

namespace xvorin::serdes {

template <typename T, typename E = void>
struct Converter {
};

// arithmetic
template <typename T>
struct Converter<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    static std::string to_string(T t)
    {
        if (std::is_same<T, bool>::value) {
            return t ? "true" : "false";
        }

        return std::to_string(t);
    }

    static T from_string(const std::string& s)
    {
        if (std::is_same<T, bool>::value) {
            return (s == "true" || s == "True" || s == "1");
        }

        std::stringstream ss;
        T t;
        ss << s;
        ss >> t;
        return t;
    }
};

// std::string
template <typename T>
struct Converter<T, typename std::enable_if<std::is_same<T, std::string>::value>::type> {
    static std::string to_string(T t)
    {
        return t;
    }

    static T from_string(const std::string& s)
    {
        return s;
    }
};

// buffer
template <typename T>
struct Converter<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> {
    static std::string to_string(T t)
    {
        return (utils::is_ascii_string(t) || utils::is_utf8_string(t)) ? t : base64_encode(t);
    }

    static T from_string(const std::string& s)
    {
        auto t = base64_decode(s);
        return t.empty() ? s : t;
    }
};

// enum
template <typename T>
struct Converter<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    static std::string to_string(const T& t)
    {
        auto proto = std::static_pointer_cast<TraitedParameter<T>>(ParameterPrototype::query_prototype(typeid(T)));
        if (!proto) {
            return std::string();
        }

        auto iter = proto->enum_mapping.find(t);
        if (iter == proto->enum_mapping.end()) {
            return std::string();
        }

        return iter->second;
    }

    static T from_string(const std::string& s)
    {
        auto proto = std::static_pointer_cast<TraitedParameter<T>>(ParameterPrototype::query_prototype(typeid(T)));
        if (!proto) {
            return static_cast<T>(0);
        }

        for (auto em : proto->enum_mapping) {
            if (em.second == s) {
                return em.first;
            }
        }

        return static_cast<T>(0);
    }
};

}