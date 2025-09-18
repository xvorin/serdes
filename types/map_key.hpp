#pragma once

#include <sstream>

namespace xvorin::serdes {

template <typename T>
inline std::string to_string(const T& t)
{
    return std::to_string(t);
}

template <typename T>
inline T from_string(const std::string& s)
{
    std::stringstream ss;
    T t;
    ss << s;
    ss >> t;
    return t;
}

template <>
inline std::string to_string(const std::string& t)
{
    return t;
}

template <>
inline std::string from_string(const std::string& s)
{
    return s;
}

template <>
inline std::string to_string(const bool& t)
{
    return t ? "true" : "false";
}

template <>
inline bool from_string(const std::string& s)
{
    return (s == "true" || s == "True" || s == "1");
}

}