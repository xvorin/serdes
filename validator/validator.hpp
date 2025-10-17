#pragma once

#include "serdes/types/traits.hpp"
#include "serdes/utils/utils.hpp"

#include <regex>

namespace xvorin::serdes {

template <typename T, typename E = void> //[E]nable
struct Validator {
};

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
        utils::split(verinfo, limits, ";");

        for (const auto& limit : limits) {
            std::vector<std::string> limit_pair;
            utils::split(limit, limit_pair, ",");
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

            const T low_value = Converter<T>::from_string(low.substr(1));
            const T high_value = Converter<T>::from_string(high.substr(0, high.size() - 1));

            if (low_value > high_value) {
                continue;
            }

            range_.push_back({ { low_value, low_hit }, { high_value, high_hit } });
        }

        std::stringstream ss;
        for (size_t i = 0; i < range_.size(); i++) {
            const auto& low = range_[i].first;
            const auto& high = range_[i].second;
            ss << (low.hit ? "[" : "(") << Converter<T>::to_string(low.value) << ","
               << Converter<T>::to_string(high.value) << (high.hit ? "]" : ")");
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
        utils::split(verinfo, limits, ",");

        for (auto& limit : limits) {
            values_.insert(Converter<int>::from_string(limit));
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
        if (!utils::is_ascii_string(value) && !utils::is_utf8_string(value)) {
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

private:
    std::string pattern_;
};

// buffer
template <typename T>
struct Validator<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> {
public:
    bool operator()(const T& value) const
    {
        return true;
    }

    void parse_verification_info(const std::string& verinfo)
    {
        pattern_ = verinfo;
    }

private:
    std::string pattern_;
};

}