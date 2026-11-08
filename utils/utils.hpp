#pragma once

#include "serdes/types/extension/envar.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

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

// 读取文件全部内容; 异常统一走 utils/exception.hpp: 路径含 ${VAR} 且环境变量缺失抛 EnvarNotExist,
// 文件缺失抛 BadFile, 均携带出问题的路径串
inline std::string read_file(const std::string& path)
{
    const auto expanded = static_cast<std::string>(envar(path));
    std::ifstream fin(expanded);
    if (!fin) {
        throw BadFile(expanded);
    }
    std::stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

// 原子写文件: 路径里的 ${VAR} 与读侧一致先展开, 再写"唯一临时文件"并 rename, 避免写一半。
// 临时名带 pid + 进程内序号: 多进程(含与其他工具并发)各自写各自的临时文件, 互不覆盖;
// rename 原子替换目标, 读者任意时刻看到的都是完整旧内容或完整新内容, 不会损坏配置文件。
// 环境变量缺失抛 EnvarNotExist, 写入失败抛 BadFile(均携带展开后的路径)
inline void write_file_atomic(const std::string& path, const std::string& content)
{
    const auto expanded = static_cast<std::string>(envar(path));

    static std::atomic<unsigned long> sequence { 0 };
    std::stringstream tmp_name;
    tmp_name << expanded << ".tmp." << static_cast<long>(::getpid()) << "." << sequence.fetch_add(1);
    const std::string tmp = tmp_name.str();

    {
        std::ofstream fout(tmp);
        if (!fout) {
            throw BadFile(tmp);
        }
        fout << content;
        fout.flush();
        if (!fout) {
            std::remove(tmp.c_str());
            throw BadFile(tmp);
        }
    }
    if (0 != std::rename(tmp.c_str(), expanded.c_str())) {
        std::remove(tmp.c_str());
        throw BadFile(expanded);
    }
}

} // namespace xvorin::serdes
