#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/serdes/yaml/yaml_util.hpp"
#include "serdes/types/traits.hpp"

#include <sstream>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class YamlSerdes : public Serdes {
};

// BASIC
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_basic<T>::value && !is_extension_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = parameter->value;
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_null()) {
            parameter->value = yin.get_value<T>();
        }
    }
};

// BASIC-buffer
template <typename T>
class YamlSerdes<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = base64_encode(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_null()) {
            parameter->value = base64_decode(yin.get_value<std::string>());
        }
    }
};

// BASIC-envar
template <typename T>
class YamlSerdes<T, typename std::enable_if<std::is_same<T, envar>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = parameter->value.original();
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_null()) {
            parameter->value = yin.get_value<std::string>();
        }
    }
};

// ENUM
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = Converter<T>::to_string(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        parameter->value = Converter<T>::from_string(yin.as_str());
    }
};

// OBJECT: 按成员名写出/合并子成员(纯内容; subtree 的内容也走这里)
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
public:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = fkyaml::ordered_node::mapping();
        for (const auto& child : p->sorted_children()) {
            auto& ychild = yout[child->subkey];
            child->serialize(&ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_mapping()) {
            return;
        }

        // 只认领与已注册子节点同名的 key
        for (const auto& child_pair : p->children()) {
            const auto& child = child_pair.second;
            if (!yin.contains(child->subkey)) {
                continue;
            }
            const auto& ychild = yin[child->subkey];
            child->deserialize(&ychild);
        }
    }
};

// BASIC-subtree: 外层是路径字符串(或内联对象); 内容部分复用上面的对象实现
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_subtree<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        if (!parameter->profile()->subtree_inline) {
            if (parameter->profile()->subtree_sink) { // 保存主配置时顺带落盘
                parameter->save(parameter->profile()->serdes_type);
            }
            yout = parameter->path; // 默认: 输出路径字符串
            return;
        }

        YamlSerdes<typename T::value_type> content; // 内联模式: 输出子树内容对象
        content.serialize(p, out);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (yin.is_scalar()) { // 路径: 加载文件内容, 合并到子树
            parameter->path = yin.as_str();
            parameter->load(ParameterSerdesType::PST_YAML);
            return;
        }

        YamlSerdes<typename T::value_type> content; // 内联对象: 直接作为子树, path 保持原值
        content.deserialize(p, in);
    }
};

// SEQUENCE
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = fkyaml::ordered_node::sequence();

        for (const auto& child : parameter->sorted_children()) {
            fkyaml::ordered_node ychild;
            child->serialize(&ychild);
            yout.as_seq().push_back(ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_sequence()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& ychild : yin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&ychild);
            children->emplace(subkey, child);
        }
    }
};

// MAP
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = fkyaml::ordered_node::mapping();

        for (const auto& child : parameter->sorted_children()) {
            auto& ychild = yout[child->subkey];
            child->serialize(&ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_mapping()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        for (const auto& ypair : yin.map_items()) {
            const auto& subkey = ypair.key().as_str();
            const auto& ychild = ypair.value();
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&ychild);
            children->emplace(subkey, child);
        }
    }
};

// SET
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));
        yout = fkyaml::ordered_node::sequence();

        for (const auto& child : parameter->sorted_children()) {
            fkyaml::ordered_node ychild;
            child->serialize(&ychild);
            yout.as_seq().push_back(ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));
        if (!yin.is_sequence()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& ychild : yin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&ychild);
            children->emplace(subkey, child);
        }
    }
};

// PTR
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<fkyaml::ordered_node*>(out));

        if (parameter->value) {
            parameter->value->serialize(out);
        } else {
            yout = nullptr;
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const fkyaml::ordered_node*>(in));

        if (!yin.is_null()) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
            parameter->value->deserialize(in);
        }
    }
};


// ---- 文本 <-> DOM(供 api 与子树内容读写共用) ----
namespace yaml {

inline fkyaml::ordered_node parse(const std::string& text)
{
    try {
        return fkyaml::ordered_node::deserialize(text);
    } catch (const std::exception& e) {
        throw ParseYamlException(e.what());
    }
}

inline std::string dump(const fkyaml::ordered_node& dom)
{
    try {
        std::stringstream ss;
        ss << dom;
        return ss.str();
    } catch (const std::exception& e) {
        throw DumpYamlException(e.what());
    }
}

}

}