#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace xvorin::serdes::utils {

/// 字符串分隔
inline void split(const std::string& str, std::vector<std::string>& result, const std::string& delim = " ")
{
    if (str.empty() || delim.empty()) {
        return;
    }

    std::string::const_iterator substart = str.begin();
    std::string::const_iterator subend = substart;
    while (true) {
        subend = std::search(substart, str.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);

        if (!temp.empty()) {
            result.push_back(temp);
        }

        if (subend == str.end()) {
            break;
        }
        substart = subend + delim.size();
    }
}

// 使用std::string的replace函数
inline std::string replace(const std::string& str, const std::string& o, const std::string& n)
{
    std::string result = str;
    size_t pos = 0;

    while ((pos = result.find(o, pos)) != std::string::npos) {
        result.replace(pos, o.length(), n);
        pos += n.length(); // 移动位置到替换后的字符串之后
    }

    return result;
}

inline bool is_ascii_string(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), [](const char& c) {
        return std::isprint(static_cast<unsigned char>(c));
    });
}

inline bool is_utf8_string(const std::string& s)
{
    for (size_t i = 0; i < s.length();) {
        const unsigned char c = s[i];

        if (c <= 0x7F) { // ASCII 字符
            if (!std::isprint(c) && c != '\t' && c != '\n' && c != '\r') {
                return false;
            }
            i++;
        } else if ((c & 0xE0) == 0xC0) { // 2字节UTF-8
            i += 2;
            if (i > s.length()) {
                return false;
            }
        } else if ((c & 0xF0) == 0xE0) { // 3字节UTF-8（包括中文）
            i += 3;
            if (i > s.length()) {
                return false;
            }
        } else if ((c & 0xF8) == 0xF0) { // 4字节UTF-8
            i += 4;
            if (i > s.length()) {
                return false;
            }
        } else {
            return false; // 无效的UTF-8序列
        }
    }
    return true;
}

} // namespace xvorin::serdes
