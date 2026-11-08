#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <serdes/3rd/nlohmann/json.hpp>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class JsonSerdes : public Serdes {
};

// BASIC
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_basic<T>::value && !is_extension_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        parameter->value = jin.is_null() ? T() : jin.get<T>();
    }
};

// BASIC-buffer
template <typename T>
class JsonSerdes<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json(base64_encode(parameter->value));
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        parameter->value = jin.is_null() ? T() : base64_decode(jin.get<std::string>());
    }
};

// BASIC-envar
template <typename T>
class JsonSerdes<T, typename std::enable_if<std::is_same<T, envar>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json(parameter->value.original());
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        // envar与std::string可互转, 三目运算符会产生歧义, 须用if/else显式区分
        if (jin.is_null()) {
            parameter->value = T();
        } else {
            parameter->value = jin.get<std::string>();
        }
    }
};

// ENUM
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json(Converter<T>::to_string(parameter->value));
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        parameter->value = jin.is_null() ? T() : Converter<T>::from_string(jin.get<std::string>());
    }
};

// OBJECT: 按成员名写出/合并子成员(纯内容; subtree 的内容也走这里)
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
public:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        for (const auto& child : p->sorted_children()) {
            nlohmann::ordered_json& jchild = jout[child->subkey];
            child->serialize(&jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        if (jin.is_null()) {
            return;
        }

        // 只认领与已注册子节点同名的 key
        for (const auto& child_pair : p->children()) {
            const auto& child = child_pair.second;
            auto iter = jin.find(child->subkey);
            if (iter != jin.end()) {
                child->deserialize(&(*iter));
            }
        }
    }
};

// BASIC-subtree: 外层是路径字符串(或内联对象); 内容部分复用上面的对象实现
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_subtree<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        if (!parameter->profile()->subtree_inline) {
            if (parameter->profile()->subtree_sink) { // 保存主配置时顺带落盘
                parameter->save(parameter->profile()->serdes_type);
            }
            jout = parameter->path; // 默认: 输出路径字符串
            return;
        }

        JsonSerdes<typename T::value_type> content; // 内联模式: 输出子树内容对象
        content.serialize(p, out);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        if (jin.is_string()) { // 路径: 加载文件内容, 合并到子树
            parameter->path = jin.get<std::string>();
            parameter->load(ParameterSerdesType::PST_JSON);
            return;
        }

        JsonSerdes<typename T::value_type> content; // 内联对象: 直接作为子树, path 保持原值
        content.deserialize(p, in);
    }
};

// SEQUENCE
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json::array();

        for (const auto& child : parameter->sorted_children()) {
            nlohmann::ordered_json jchild;
            child->serialize(&jchild);
            jout.push_back(jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        if (!jin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();
        if (jin.is_null()) {
            return;
        }

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&jchild);
            children->emplace(subkey, child);
        }
    }
};

// MAP
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));

        for (const auto& child : parameter->sorted_children()) {
            auto& jchild = jout[child->subkey];
            child->serialize(&jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));

        auto children = parameter->mutable_children();
        children->clear();
        if (jin.is_null()) {
            return;
        }

        for (const auto& jpair : jin.items()) {
            const auto& subkey = jpair.key();
            const auto& jchild = jpair.value();
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&jchild);
            children->emplace(subkey, child);
        }
    }
};

// SET
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));
        jout = nlohmann::ordered_json::array();

        for (const auto& child : parameter->sorted_children()) {
            nlohmann::ordered_json jchild;
            child->serialize(&jchild);
            jout.push_back(jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));
        if (!jin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();
        if (jin.is_null()) {
            return;
        }

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&jchild);
            children->emplace(subkey, child);
        }
    }
};

// PTR
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));

        if (parameter->value) {
            parameter->value->serialize(out);
        } else {
            jout = nullptr;
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));

        if (jin != nullptr) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
            parameter->value->deserialize(&jin);
        }
    }
};


// ---- 文本 <-> DOM(供 api 与子树内容读写共用) ----
namespace json {

inline nlohmann::ordered_json parse(const std::string& text)
{
    try {
        return nlohmann::ordered_json::parse(text);
    } catch (const std::exception& e) {
        throw ParseJsonException(e.what());
    }
}

inline std::string dump(const nlohmann::ordered_json& dom, int indent = 4)
{
    try {
        return indent > 0 ? dom.dump(indent) : dom.dump();
    } catch (const std::exception& e) {
        throw DumpJsonException(e.what());
    }
}

}

}