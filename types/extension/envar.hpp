#pragma once

#include <cstdlib>
#include <regex>
#include <string>

#include <iostream>

namespace xvorin::serdes {

class envar {

public:
    // 构造函数：接收 std::string
    envar(std::string str)
        : original_(std::move(str))
    {
        replace_envs();
    }

    // 构造函数：接收 const char*
    envar(const char* str)
        : envar(std::string(str))
    {
    }

    envar() = default;
    envar(const envar& other) = default; // 拷贝构造函数
    envar(envar&& other) = default; // 移动构造函数

    envar& operator=(const envar& other) = default; // 赋值运算符
    envar& operator=(envar&& other) = default; // 移动赋值运算符

    bool operator==(const envar& other)
    {
        return original_ == other.original_ && result_ == other.result_;
    }

    // std::string
    operator std::string() const
    {
        return result_;
    }

    // 流输出运算符
    friend std::ostream& operator<<(std::ostream& os, const envar& es)
    {
        os << static_cast<std::string>(es);
        return os;
    }

    // 获取原始字符串
    const std::string& original() const
    {
        return original_;
    }

private:
    // 环境变量替换实现
    void replace_envs()
    {
        const auto& original = original_;
        if (original.empty()) {
            return;
        }

        std::string result;
        static const std::regex pattern(R"(\$\{([^}]+)\})");

        size_t substart = 0;
        for (auto iter = std::sregex_iterator(original.cbegin(), original.cend(), pattern);
            iter != std::sregex_iterator(); iter++) {
            const auto env = (*iter)[1].str();
            const char* value = std::getenv(env.c_str());
            if (value == nullptr) { // 环境变量不存在
                throw EnvarNotExist(env);
            }
            result.append(original.substr(substart, iter->position() - substart)).append(value);
            substart = iter->position() + iter->length();
        }
        result.append(original.substr(substart, original.length()));

        result_.swap(result);
    }

private:
    std::string original_;
    std::string result_;
};

}