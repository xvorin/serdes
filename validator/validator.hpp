#pragma once

#include "serdes/types/traits.hpp"
#include <regex>

namespace xvorin::serdes {

template <typename T, typename E = void> //[E]nable
struct Validator {
};

namespace {

/// 字符串分隔，insertEmpty : 如果有连续的delim，是否插入空串
inline void split(const std::string& str, std::vector<std::string>& result, const std::string& delim = " ",
    bool insertEmpty = false)
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
        } else if (insertEmpty) {
            result.push_back("");
        }

        if (subend == str.end())
            break;
        substart = subend + delim.size();
    }
}

}

// 数字
template <typename T>
struct Validator<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
public:
    bool operator()(const T& value) const
    {
        if (range_.empty()) {
            return true;
        }

        for (auto& p : range_) {
            const auto low = (p.first.hit && value >= p.first.value) || value > p.first.value;
            const auto high = (p.second.hit && value <= p.second.value) || value < p.second.value;

            if (low && high) {
                return true;
            }
        }

        return false;
    }

    void parse_verification_info(const std::string& verinfo)
    {
        // (1,3);[4,6)
        std::vector<std::string> limits;
        split(verinfo, limits, ";");

        for (const auto& limit : limits) {
            std::vector<std::string> limit_pair;
            split(limit, limit_pair, ",");
            if (limit_pair.size() != 2) {
                continue;
            }

            const std::string& low = limit_pair[0];
            const std::string& high = limit_pair[1];
            bool low_hit = false, high_hit = false;

            if ('(' == low[0]) {
                low_hit = false;
            } else if ('[' == low[0]) {
                low_hit = true;
            } else {
                continue;
            }

            if (')' == high.back()) {
                high_hit = false;
            } else if (']' == high.back()) {
                high_hit = true;
            } else {
                continue;
            }

            const T low_value = from_string<T>(low.substr(1));
            const T high_value = from_string<T>(high.substr(0, high.size() - 1));

            if (low_value > high_value) {
                continue;
            }

            range_.push_back({ { low_value, low_hit }, { high_value, high_hit } });
        }

        std::stringstream ss;
        for (size_t i = 0; i < range_.size(); i++) {
            const auto& low = range_[i].first;
            const auto& high = range_[i].second;
            ss << (low.hit ? "[" : "(") << to_string<T>(low.value) << ","
               << to_string<T>(high.value) << (high.hit ? "]" : ")");
            if (i < range_.size() - 1) {
                ss << ";";
            }
        }

        const_cast<std::string&>(verinfo) = ss.str();
    }

private:
    struct Range {
        T value;
        bool hit;
    };
    std::vector<std::pair<Range, Range>> range_;
};

// 枚举
template <typename T>
struct Validator<T, typename std::enable_if<std::is_enum<T>::value>::type> {
public:
    bool operator()(const T& value) const
    {
        if (values_.empty()) {
            return true;
        }

        return values_.find(static_cast<int>(value)) != values_.end();
    }

    void parse_verification_info(const std::string& verinfo)
    {
        std::vector<std::string> limits;
        split(verinfo, limits, ",");

        for (auto& limit : limits) {
            values_.insert(from_string<int>(limit));
        }
    }

private:
    std::set<int> values_;
};

// 字符串
template <typename T>
struct Validator<T, typename std::enable_if<std::is_same<T, std::string>::value>::type> {
public:
    bool operator()(const T& value) const
    {
        if (!is_ascii_string(value) && !is_utf8_string(value)) {
            return false;
        }

        if (pattern_.empty()) {
            return true;
        }

        std::smatch result;
        std::regex pattern(pattern_);
        return std::regex_match(value, result, pattern);
    }

    void parse_verification_info(const std::string& verinfo)
    {
        pattern_ = verinfo;
    }

    bool is_ascii_string(const std::string& s) const
    {
        return std::all_of(s.begin(), s.end(), [](const char& c) {
            return (0 == (c & 0x80));
        });
    }

    bool is_utf8_string(const std::string& s) const
    {
        int check_sub = 0;
        for (auto c : s) {
            if (check_sub == 0) {
                if ((c >> 7) == 0) { // 0xxx xxxx
                    continue;
                } else if ((c & 0xE0) == 0xC0) { // 110x xxxx
                    check_sub = 1;
                } else if ((c & 0xF0) == 0xE0) { // 1110 xxxx
                    check_sub = 2;
                } else if ((c & 0xF8) == 0xF0) { // 1111 0xxx
                    check_sub = 3;
                } else if ((c & 0xFC) == 0xF8) { // 1111 10xx
                    check_sub = 4;
                } else if ((c & 0xFE) == 0xFC) { // 1111 110x
                    check_sub = 5;
                } else {
                    return false;
                }
            } else {
                if ((c & 0xC0) != 0x80) {
                    return false;
                }
                check_sub--;
            }
        }
        return true;
    }

private:
    std::string pattern_;
};

}