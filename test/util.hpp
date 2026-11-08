#pragma once

#include "serdes/serdes.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ---- 简易断言: 失败时打印文件:行号并退出(返回非零, 供 ctest 判定失败) ----
#define CHECK(cond)                                                                           \
    do {                                                                                      \
        if (!(cond)) {                                                                        \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << "  " #cond << std::endl; \
            exit(1);                                                                          \
        }                                                                                     \
    } while (0)

#define CHECK_MSG(cond, msg)                                                    \
    do {                                                                        \
        if (!(cond)) {                                                          \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << "  " #cond \
                      << "  (" << (msg) << ")" << std::endl;                    \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define CHECK_STR_CONTAINS(haystack, needle)                                       \
    do {                                                                           \
        const std::string& hs_ = (haystack);                                       \
        const std::string nd_ = (needle);                                          \
        if (hs_.find(nd_) == std::string::npos) {                                  \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << "  \"" << nd_ \
                      << "\" not found in: " << hs_ << std::endl;                  \
            exit(1);                                                               \
        }                                                                          \
    } while (0)

#define CHECK_THROW(expr, ExceptionType)                                                      \
    do {                                                                                      \
        bool threw_ = false;                                                                  \
        try {                                                                                 \
            expr;                                                                             \
        } catch (const ExceptionType& e_) {                                                   \
            threw_ = true;                                                                    \
            (void)e_;                                                                         \
        } catch (const std::exception& e_) {                                                  \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__                             \
                      << "  expected " #ExceptionType " but got: " << e_.what() << std::endl; \
            exit(1);                                                                          \
        }                                                                                     \
        if (!threw_) {                                                                        \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__                             \
                      << "  expected " #ExceptionType " but nothing thrown" << std::endl;     \
            exit(1);                                                                          \
        }                                                                                     \
    } while (0)

namespace testutil {

// 清理并重建 /tmp/serdes_test/<name>, 返回目录路径
inline std::string make_dir(const std::string& name)
{
    const std::string path = "/tmp/serdes_test/" + name;
    const std::string cmd = "rm -rf " + path + " && mkdir -p " + path;
    if (0 != system(cmd.c_str())) {
        std::cerr << "[FAIL] cannot prepare dir " << path << std::endl;
        exit(1);
    }
    return path;
}

inline void write_file(const std::string& path, const std::string& content)
{
    std::ofstream fout(path);
    if (!fout) {
        std::cerr << "[FAIL] cannot write " << path << std::endl;
        exit(1);
    }
    fout << content;
}

inline std::string read_file(const std::string& path)
{
    std::ifstream fin(path);
    std::stringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

}
