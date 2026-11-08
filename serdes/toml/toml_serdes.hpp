#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <serdes/3rd/toml11/toml.hpp>

#include <sstream>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class TomlSerdes : public Serdes {
};

// BASIC
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_basic<T>::value && !is_extension_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = parameter->value;

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    template <typename V>
    void from_toml_value(V& out, const ::toml::ordered_value& in)
    {
        out = in.as_integer();
    }

    void from_toml_value(std::string& out, const ::toml::ordered_value& in)
    {
        out = in.as_string();
    }

    void from_toml_value(bool& out, const ::toml::ordered_value& in)
    {
        out = in.as_boolean();
    }

    void from_toml_value(float& out, const ::toml::ordered_value& in)
    {
        out = in.as_floating();
    }

    void from_toml_value(double& out, const ::toml::ordered_value& in)
    {
        out = in.as_floating();
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        from_toml_value(parameter->value, tin);
    }
};

// BASIC-buffer
template <typename T>
class TomlSerdes<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = base64_encode(parameter->value);

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        parameter->value = base64_decode(tin.as_string());
    }
};

// BASIC-envar
template <typename T>
class TomlSerdes<T, typename std::enable_if<std::is_same<T, envar>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = parameter->value.original();

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        parameter->value = tin.as_string();
    }
};

// ENUM
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = Converter<T>::to_string(parameter->value);

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        parameter->value = Converter<T>::from_string(tin.as_string());
    }
};

// OBJECT: 按成员名写出/合并子成员(纯内容; subtree 的内容也走这里)
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
public:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = ::toml::ordered_table();
        for (const auto& child : p->sorted_children()) {
            auto& tchild = tout[child->subkey];
            child->serialize(&tchild);
        }

        if (!p->comment.empty()) {
            tout.comments().push_back(p->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        if (!tin.is_table()) {
            return;
        }

        // 只认领与已注册子节点同名的 key
        for (const auto& child_pair : p->children()) {
            const auto& child = child_pair.second;
            if (!tin.contains(child->subkey)) {
                continue;
            }
            const auto& tchild = tin.at(child->subkey);
            child->deserialize(&tchild);
        }
    }
};

// BASIC-subtree: 外层是路径字符串(或内联对象); 内容部分复用上面的对象实现
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_subtree<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        if (!parameter->profile()->subtree_inline) {
            if (parameter->profile()->subtree_sink) { // 保存主配置时顺带落盘
                parameter->save(parameter->profile()->serdes_type);
            }
            tout = parameter->path; // 默认: 输出路径字符串
            if (!parameter->comment.empty()) {
                tout.comments().push_back(parameter->comment);
            }
            return;
        }

        TomlSerdes<typename T::value_type> content; // 内联模式: 输出子树内容对象(含注释)
        content.serialize(p, out);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        if (tin.is_string()) { // 路径: 加载文件内容, 合并到子树
            parameter->path = tin.as_string();
            parameter->load(ParameterSerdesType::PST_TOML);
            return;
        }

        TomlSerdes<typename T::value_type> content; // 内联对象: 直接作为子树, path 保持原值
        content.deserialize(p, in);
    }
};

// SEQUENCE
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = ::toml::ordered_array();

        for (const auto& child : parameter->sorted_children()) {
            ::toml::ordered_value tchild;
            child->serialize(&tchild);
            tout.as_array().push_back(tchild);
        }

        // if (!parameter->comment.empty()) {
        //     tout.comments().push_back(parameter->comment);
        // }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        if (!tin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&tchild);
            children->emplace(subkey, child);
        }
    }
};

// MAP
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = ::toml::ordered_table();

        for (const auto& child : parameter->sorted_children()) {
            auto& tchild = tout[child->subkey];
            child->serialize(&tchild);
        }

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        if (!tin.is_table()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        for (const auto& tpair : tin.as_table()) {
            const auto& subkey = tpair.first;
            const auto& tchild = tpair.second;
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&tchild);
            children->emplace(subkey, child);
        }
    }
};

// SET
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));
        tout = ::toml::ordered_array();

        for (const auto& child : parameter->sorted_children()) {
            ::toml::ordered_value tchild;
            child->serialize(&tchild);
            tout.as_array().push_back(tchild);
        }

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));
        if (!tin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&tchild);
            children->emplace(subkey, child);
        }
    }
};

// PTR
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<::toml::ordered_value*>(out));

        if (parameter->value) {
            parameter->value->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const ::toml::ordered_value*>(in));

        if (!tin.is_empty()) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
            parameter->value->deserialize(in);
        }
    }
};


// ---- 文本 <-> DOM(供 api 与子树内容读写共用) ----
// 注意: 本文件位于 xvorin::serdes::toml 内, 引用 toml11 必须写全 ::toml::
namespace toml {

inline ::toml::ordered_value parse(const std::string& text)
{
    try {
        auto spec = ::toml::spec::v(1, 0, 0);
        spec.ext_null_value = true; // 允许 `key = null`
        return ::toml::parse_str(text, spec);
    } catch (const std::exception& e) {
        throw ParseTomlException(e.what());
    }
}

inline std::string dump(const ::toml::ordered_value& dom)
{
    try {
        std::stringstream ss;
        auto spec = ::toml::spec::v(1, 0, 0);
        spec.ext_null_value = true;
        ss << ::toml::format(dom, spec);
        return ss.str();
    } catch (const std::exception& e) {
        throw DumpTomlException(e.what());
    }
}

}

}