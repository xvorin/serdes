#pragma once

#include <algorithm>
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

} // namespace xvorin::serdes
