#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"
#include "serdes/utils/converter.hpp"

#include <serdes/3rd/pugixml/pugixml.hpp>

#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace xvorin::serdes {

// XML映射规则:
//   元素名 = 参数subkey(可作合法XML标签名时); 否则标签名用param并以name属性记录原始subkey
//   基础/枚举/buffer/envar类型 = 元素文本; OBJECT/MAP = 子元素; SEQUENCE/SET = <item>子元素
//   参数comment = XML注释节点(挂接在元素之前, 与toml的"#注释"行风格一致)
//   反序列化时pugixml默认跳过注释, 注释始终来自参数注册信息

// 判断字符串能否直接作为XML元素名: 首字符为字母/下划线/冒号, 其余为字母/数字/下划线/冒号/点/横线
inline bool xml_valid_name(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    const auto head_ok = std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_' || name[0] == ':';
    if (!head_ok) {
        return false;
    }

    for (size_t i = 1; i < name.size(); i++) {
        const auto ok = std::isalnum(static_cast<unsigned char>(name[i])) || name[i] == '_' || name[i] == ':' || name[i] == '.' || name[i] == '-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

// 读取元素对应的subkey: 优先取name属性(非法标签名时的兜底), 否则取标签名
inline std::string xml_subkey_of(const pugi::xml_node& node)
{
    const auto name = node.attribute("name");
    return name ? name.as_string() : node.name();
}

// 挂接子元素: subkey合法时直接作标签名, 否则用固定标签param + name属性记录原始subkey
inline pugi::xml_node xml_append_element(pugi::xml_node& parent, const std::string& subkey)
{
    if (xml_valid_name(subkey)) {
        return parent.append_child(subkey.c_str());
    }

    auto child = parent.append_child("param");
    child.append_attribute("name").set_value(subkey.c_str());
    return child;
}

// 将注释挂接到元素之前(与toml的"#注释"行风格一致); 根元素无父节点时挂接为其子节点
inline void xml_set_comment(pugi::xml_node& node, const std::string& comment)
{
    if (comment.empty()) {
        return;
    }

    auto parent = node.parent();
    if (parent) {
        parent.insert_child_before(pugi::node_comment, node).set_value(comment.c_str());
    } else {
        node.append_child(pugi::node_comment).set_value(comment.c_str());
    }
}

// ---- 数值/文本转换 ----

// 浮点数取最短可无损往返的十进制表示(与json/toml输出风格一致); 非有限值按默认精度输出
template <typename T>
inline std::string xml_float_to_text(T v)
{
    const auto max_digits = std::numeric_limits<T>::max_digits10;

    std::stringstream fallback;
    fallback << std::setprecision(max_digits) << v;
    if (!std::isfinite(v)) {
        return fallback.str();
    }

    for (int prec = 1; prec < max_digits; prec++) {
        std::stringstream ss;
        ss << std::setprecision(prec) << v;

        T back { };
        std::stringstream in(ss.str());
        if ((in >> back) && back == v) {
            return ss.str();
        }
    }

    return fallback.str();
}

inline std::string xml_to_text(bool v)
{
    return v ? "true" : "false";
}

inline std::string xml_to_text(const std::string& v)
{
    return v;
}

inline std::string xml_to_text(double v)
{
    return xml_float_to_text(v);
}

inline std::string xml_to_text(float v)
{
    return xml_float_to_text(v);
}

// 其余算术类型(整型)走此模板
template <typename T>
inline std::string xml_to_text(T v)
{
    return std::to_string(v);
}

inline void xml_from_text(bool& out, const std::string& s)
{
    out = (s == "true" || s == "True" || s == "1");
}

inline void xml_from_text(std::string& out, const std::string& s)
{
    out = s;
}

// char/int8_t/uint8_t按整数解析, 避免流提取按单字符读取
inline void xml_from_text(char& out, const std::string& s)
{
    long long tmp { };
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = static_cast<char>(tmp);
    }
}

inline void xml_from_text(signed char& out, const std::string& s)
{
    long long tmp { };
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = static_cast<signed char>(tmp);
    }
}

inline void xml_from_text(unsigned char& out, const std::string& s)
{
    long long tmp { };
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = static_cast<unsigned char>(tmp);
    }
}

// 其余算术类型(整型/浮点)走此模板; 解析失败时保持原值
template <typename T>
inline void xml_from_text(T& out, const std::string& s)
{
    T tmp { };
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = tmp;
    }
}

template <typename T, typename E = void> //[T]ype & [E]nable
class XmlSerdes : public Serdes {
};

// BASIC
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_basic<T>::value && !is_extension_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        xout.text() = xml_to_text(parameter->value).c_str();

        xml_set_comment(xout, parameter->comment);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));
        xml_from_text(parameter->value, xin.text().as_string());
    }
};

// BASIC-buffer
template <typename T>
class XmlSerdes<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        xout.text() = base64_encode(parameter->value).c_str();

        xml_set_comment(xout, parameter->comment);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));
        parameter->value = base64_decode(xin.text().as_string());
    }
};

// BASIC-envar
template <typename T>
class XmlSerdes<T, typename std::enable_if<std::is_same<T, envar>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        xout.text() = parameter->value.original().c_str();

        xml_set_comment(xout, parameter->comment);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));
        parameter->value = xin.text().as_string();
    }
};

// ENUM
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        xout.text() = Converter<T>::to_string(parameter->value).c_str();

        xml_set_comment(xout, parameter->comment);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));
        parameter->value = Converter<T>::from_string(xin.text().as_string());
    }
};

// OBJECT
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));

        xml_set_comment(xout, parameter->comment);

        for (const auto& child : parameter->sorted_children()) {
            auto xchild = xml_append_element(xout, child->subkey);
            child->serialize(&xchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        const auto& children = parameter->children();
        for (auto xchild = xin.first_child(); xchild; xchild = xchild.next_sibling()) {
            if (xchild.type() != pugi::node_element) {
                continue;
            }

            auto iter = children.find(xml_subkey_of(xchild));
            if (iter != children.end()) {
                iter->second->deserialize(&xchild);
            }
        }
    }
};

// SEQUENCE
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));

        xml_set_comment(xout, parameter->comment);

        for (const auto& child : parameter->sorted_children()) {
            auto xchild = xout.append_child("item");
            child->serialize(&xchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (auto xchild = xin.first_child(); xchild; xchild = xchild.next_sibling()) {
            if (xchild.type() != pugi::node_element) {
                continue;
            }

            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&xchild);
            children->emplace(subkey, child);
        }
    }
};

// MAP
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));

        xml_set_comment(xout, parameter->comment);

        for (const auto& child : parameter->sorted_children()) {
            auto xchild = xml_append_element(xout, child->subkey);
            child->serialize(&xchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        auto children = parameter->mutable_children();
        children->clear();

        for (auto xchild = xin.first_child(); xchild; xchild = xchild.next_sibling()) {
            if (xchild.type() != pugi::node_element) {
                continue;
            }

            const auto subkey = xml_subkey_of(xchild);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&xchild);
            children->emplace(subkey, child);
        }
    }
};

// SET
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));

        xml_set_comment(xout, parameter->comment);

        for (const auto& child : parameter->sorted_children()) {
            auto xchild = xout.append_child("item");
            child->serialize(&xchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (auto xchild = xin.first_child(); xchild; xchild = xchild.next_sibling()) {
            if (xchild.type() != pugi::node_element) {
                continue;
            }

            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&xchild);
            children->emplace(subkey, child);
        }
    }
};

// PTR
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);

        if (parameter->value) {
            parameter->value->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        // 空元素表示空指针; 与toml的is_empty()语义一致
        if (xin.first_child() || !xin.text().empty()) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
            parameter->value->deserialize(in);
        }
    }
};

}
