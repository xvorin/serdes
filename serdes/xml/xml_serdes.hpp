#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/serdes/xml/xml_util.hpp"
#include "serdes/types/traits.hpp"
#include "serdes/utils/converter.hpp"

#include <serdes/3rd/pugixml/pugixml.hpp>

#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace xvorin::serdes {

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

        T back {};
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
    long long tmp {};
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = static_cast<char>(tmp);
    }
}

inline void xml_from_text(signed char& out, const std::string& s)
{
    long long tmp {};
    std::stringstream ss;
    ss << s;
    if (ss >> tmp) {
        out = static_cast<signed char>(tmp);
    }
}

inline void xml_from_text(unsigned char& out, const std::string& s)
{
    long long tmp {};
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
    T tmp {};
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

// OBJECT(含 subtree 节点: 默认元素文本为引用路径, subtree_inline 时输出子树子元素)
// OBJECT: 按成员名写出/合并子成员(纯内容; subtree 的内容也走这里)
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
public:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        xml_set_comment(xout, p->comment);
        for (const auto& child : p->sorted_children()) {
            auto xchild = xml_append_element(xout, child->subkey);
            child->serialize(&xchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto& xin = (*static_cast<const pugi::xml_node*>(in));

        // 只认领与已注册子节点同名的元素
        const auto& children = p->children();
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

// BASIC-subtree: 外层是路径字符串(或内联对象); 内容部分复用上面的对象实现
template <typename T>
class XmlSerdes<T, typename std::enable_if<is_subtree<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& xout = (*static_cast<pugi::xml_node*>(out));
        if (!parameter->profile()->subtree_inline) {
            if (parameter->profile()->subtree_sink) { // 保存主配置时顺带落盘
                parameter->save(parameter->profile()->serdes_type);
            }
            xout.text() = parameter->path.c_str(); // 默认: 输出路径字符串
            xml_set_comment(xout, parameter->comment);
            return;
        }

        XmlSerdes<typename T::value_type> content; // 内联模式: 输出子树子元素(含注释)
        content.serialize(p, out);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& xin = (*static_cast<const pugi::xml_node*>(in));
        const auto text = std::string(xin.text().as_string());
        if (!text.empty()) { // 路径: 加载文件内容, 合并到子树
            parameter->path = text;
            parameter->load(ParameterSerdesType::PST_XML); // 文件根元素下的子元素即配置各 key
            return;
        }

        XmlSerdes<typename T::value_type> content; // 内联对象: 直接作为子树, path 保持原值
        content.deserialize(p, in);
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


// ---- 文本 <-> DOM(供 api 与子树内容读写共用) ----
namespace xml {

inline pugi::xml_document parse(const std::string& text)
{
    pugi::xml_document doc;
    const auto result = doc.load_string(text.c_str());
    if (!result) {
        throw ParseXmlException(result.description());
    }
    if (!doc.document_element()) { // XML 要求唯一根元素
        throw ParseXmlException("no root element");
    }
    return doc;
}

inline std::string dump(const pugi::xml_document& dom, int indent = 4)
{
    try {
        std::stringstream ss;
        const auto flags = indent > 0 ? pugi::format_indent : pugi::format_raw;
        const auto indent_str = indent > 0 ? std::string(static_cast<size_t>(indent), ' ') : std::string();
        dom.save(ss, indent_str.c_str(), flags);
        return ss.str();
    } catch (const std::exception& e) {
        throw DumpXmlException(e.what());
    }
}

}

}