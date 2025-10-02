#pragma once

#include "serdes/exception.h"

#include <algorithm>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include <cxxabi.h>

namespace xvorin::serdes {

// 配置参数树整体行为
struct ParameterProfile {
    ParameterSerdesType serdes_type = ParameterSerdesType::PST_STRU; // 序列化行为
    bool inform_enabled = false; // 是否通知变更
    // bool invalid = false; // 非法变更标记
    std::unordered_map<std::string, std::string> violations;
};

// 参数变更通知类型
enum class ParameterInformType {
    PIT_CREATE, // 参数添加
    PIT_REMOVE, // 参数删除
    PIT_CHANGE, // 参数变更
};

// 参数类型
enum class ParameterType {
    PT_BASIC,
    PT_ENUM,
    PT_OBJECT,
    PT_SEQUENCE, // vector or list
    PT_MAP, // map or unordered_map
    PT_SET, // set or unordered_set
    PT_PTR, // raw pointer & smart pointer
};

// 参数详细类型(对type_info的包装)
using ParameterDetailType = std::reference_wrapper<const std::type_info>;

struct ParameterDetailTypeHash {
    std::size_t operator()(ParameterDetailType code) const
    {
        return code.get().hash_code();
    }
};

struct ParameterDetailTypeEqual {
    bool operator()(ParameterDetailType lhs, ParameterDetailType rhs) const
    {
        return lhs.get() == rhs.get();
    }
};

// 参数基类
class Parameter : public std::enable_shared_from_this<Parameter> {
public:
    Parameter(std::string subkey, ParameterType type, ParameterDetailType detail,
        size_t offset, std::string comment, std::string verinfo)
        : subkey(std::move(subkey))
        , type(type)
        , detail(detail)
        , offset(offset)
        , comment(std::move(comment))
        , verinfo(std::move(verinfo))
    {
    }

    virtual ~Parameter() = default;

    /**
     * 原型模式,获取已经注册的复杂参数结构
     */
    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const = 0;

    /**
     * 数据的序列化与反序列化
     * **序列化指从 内部数据结构 转换为 外部结构
     * **反序列化指从 外部结构 转换为 内部数据结构
     * **以上外部结构包括但不限于 C/C++对象、JSON、YAML、TOML、Protobuf 等
     */
    virtual void serialize(void* out) const = 0;
    virtual void deserialize(const void* in) = 0;

    /**
     * 只有基础类型需要实现此方法, 用于value的直接设置
     */
    virtual void from_string(const std::string& in) { }

    /**
     * 只有复合类型需要实现以下子节点相关接口
     */
    virtual void create_child(const std::string& newkey) { }
    virtual void remove_child(const std::string& subkey) { }
    virtual std::shared_ptr<Parameter> find_child(const std::string& subkey) const { return nullptr; }
    virtual const std::unordered_map<std::string, std::shared_ptr<Parameter>>& children() const
    {
        static const std::unordered_map<std::string, std::shared_ptr<Parameter>> empty_children;
        return empty_children;
    }
    virtual std::unordered_map<std::string, std::shared_ptr<Parameter>>* mutable_children()
    {
        return nullptr;
    }

    /**
     * 参数变更通知
     */
    virtual void inform_ancestor(const std::string& index, ParameterInformType pit) const { }

    std::string readable_type() const { return readable_type(type); }
    std::string readable_detail_type() const { return readable_detail_type(detail); }

    /**
     * 获取参数索引
     */
    std::string index() const
    {
        auto p = parent.lock();
        return p ? p->index() + "." + subkey : subkey;
    }

    /**
     * 获取参数树整体行为
     */
    std::shared_ptr<ParameterProfile> profile() const
    {
        if (profile_) {
            return profile_;
        }

        auto p = parent.lock();
        if (p && p->profile()) {
            return profile_ = p->profile();
        }

        throw NeedRootProfile();
    }

    /**
     * 参数的调试信息
     */
    virtual std::string debug_self() const = 0;
    virtual std::string debug_releation(const std::string& prefix = "") const = 0;

    /**
     * 获取ParameterType可读字符串
     */
    static std::string readable_type(ParameterType type)
    {
        static const std::unordered_map<ParameterType, std::string> maptbl = {
            { ParameterType::PT_BASIC, "basic" },
            { ParameterType::PT_ENUM, "enum" },
            { ParameterType::PT_OBJECT, "object" },
            { ParameterType::PT_SEQUENCE, "sequence" },
            { ParameterType::PT_MAP, "map" },
            { ParameterType::PT_SET, "set" },
        };
        auto iter = maptbl.find(type);
        return iter == maptbl.end() ? "unknown" : iter->second;
    }

    /**
     * 获取ParameterDetailType可读字符串
     */
    static std::string readable_detail_type(const ParameterDetailType& type)
    {
        const auto raw = std::string(abi::__cxa_demangle(type.get().name(), 0, 0, 0));

        auto remove_angle_brackets = [](std::string raw, const std::string& key) -> std::string {
            auto begin = raw.find(key);
            while (begin != std::string::npos) {
                int symbol_count = 1;
                auto end = begin + key.length();
                for (; end < raw.size(); end++) {
                    if (raw[end] == '<') {
                        symbol_count++;
                    }
                    if (raw[end] == '>') {
                        symbol_count--;
                    }
                    if (0 == symbol_count) {
                        break;
                    }
                }
                raw = raw.substr(0, begin) + raw.substr(end + 1);
                begin = raw.find(key);
            }
            return raw;
        };

        auto remove_space = [](std::string raw) -> std::string {
            if (raw.find("<") == std::string::npos) {
                return raw;
            }
            raw.erase(std::remove_if(raw.begin(), raw.end(), ::isspace), raw.end());
            return raw;
        };

        auto replace = [](std::string raw, const std::string& src, const std::string& dst) -> std::string {
            auto begin = raw.find(src);
            while (begin != std::string::npos) {
                auto end = begin + src.length();
                raw = raw.substr(0, begin) + dst + raw.substr(end);
                begin = raw.find(src);
            }
            return raw;
        };

        auto pretty = remove_angle_brackets(raw, ", std::allocator<");
        pretty = remove_angle_brackets(pretty, ", std::less<");
        pretty = remove_angle_brackets(pretty, ", std::greater<");
        pretty = remove_angle_brackets(pretty, ", std::less_equal<");
        pretty = remove_angle_brackets(pretty, ", std::greater_equal<");
        pretty = remove_angle_brackets(pretty, ", std::hash<");
        pretty = remove_angle_brackets(pretty, ", std::equal_to<");
        pretty = remove_angle_brackets(pretty, ", std::default_delete<");
        pretty = remove_space(pretty);
        pretty = replace(pretty, "std::__cxx11::basic_string<char,std::char_traits<char>>", "std::string");
        pretty = replace(pretty, "__cxx11::", "");

        return pretty;
    }

private:
    template <typename T>
    friend class ParameterTree; // ONLY init_profile by ParameterTree

    void init_profile(ParameterSerdesType type = ParameterSerdesType::PST_STRU, bool enable = false)
    {
        auto p = parent.lock();
        if (p) {
            throw ProfileOnlyRoot(index());
        }

        if (!profile_) {
            profile_ = std::make_shared<ParameterProfile>();
        }

        profile_->serdes_type = type;
        profile_->inform_enabled = enable;
    }

public:
    const std::string subkey; // parameter index name
    const ParameterType type; // parameter type
    const ParameterDetailType detail; // parameter detail type (type_info)
    const size_t offset; // parameter offset in Parent Struct
    const std::string comment; // parameter comment
    const std::string verinfo; // verification info for validator

    std::weak_ptr<Parameter> parent;

private:
    // reference to root, for tree scoped configuration
    mutable std::shared_ptr<ParameterProfile> profile_ = nullptr;
};

inline bool sort_parameter_by_offset(std::shared_ptr<Parameter> a, std::shared_ptr<Parameter> b)
{
    return a->offset < b->offset;
}

inline bool sort_parameter_by_subkey(std::shared_ptr<Parameter> a, std::shared_ptr<Parameter> b)
{
    return a->subkey < b->subkey;
}

inline bool sort_parameter_by_number(std::shared_ptr<Parameter> a, std::shared_ptr<Parameter> b)
{
    return std::stoll(a->subkey) < std::stoll(b->subkey);
}

inline bool sort_parameter_by_suffix_number(std::shared_ptr<Parameter> a, std::shared_ptr<Parameter> b)
{
    std::string aprefix, bprefix;
    for (auto r = a->subkey.rbegin(); r != a->subkey.rend(); ++r) {
        if (std::isdigit(*r)) {
            aprefix.push_back(*r);
        } else {
            break;
        }
    }

    for (auto r = b->subkey.rbegin(); r != b->subkey.rend(); ++r) {
        if (std::isdigit(*r)) {
            bprefix.push_back(*r);
        } else {
            break;
        }
    }

    if (aprefix.empty() || bprefix.empty()) {
        return sort_parameter_by_subkey(a, b);
    }

    std::reverse(aprefix.begin(), aprefix.end());
    std::reverse(bprefix.begin(), bprefix.end());

    return std::stoll(aprefix) < std::stoll(bprefix);
}

// 类型萃取主模板
template <typename T, typename E = void>
struct TraitedParameter {
};

}