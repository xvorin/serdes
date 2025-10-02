#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <nlohmann/json.hpp>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class JsonSerdes : public Serdes {
};

// BASIC
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
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

        parameter->value = jin.get<T>();
    }
};

// ENUM
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
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

        parameter->value = jin.get<T>();
    }
};

// OBJECT
template <typename T>
class JsonSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& jout = (*static_cast<nlohmann::ordered_json*>(out));

        for (const auto& child : parameter->sorted_children()) {
            nlohmann::ordered_json& jchild = jout[child->subkey];
            child->serialize(&jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));

        for (const auto& child_pair : parameter->children()) {
            const auto& child = child_pair.second;

            if (jin.find(child->subkey) == jin.end()) {
                continue;
            }

            const auto& jchild = jin[child->subkey];
            child->deserialize(&jchild);
        }
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

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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

        for (const auto& jpair : jin.items()) {
            const auto subkey = jpair.key();
            auto& jchild = jpair.value();
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0");
            parameter->value->parent = parameter;
            parameter->value->deserialize(&jin);
        }
    }
};

}