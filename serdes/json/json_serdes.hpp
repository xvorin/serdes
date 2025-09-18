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

        for (auto& child : parameter->sorted_children()) {
            nlohmann::ordered_json& jchild = jout[child->subkey];
            child->serialize(&jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));

        for (auto& cpair : parameter->children) {
            auto& child = cpair.second;

            if (jin.find(child->subkey) == jin.end()) {
                continue;
            }

            auto& jchild = jin[child->subkey];
            if (child->type == ParameterType::PT_OBJECT) {
                auto object = ParameterPrototype::create_parameter(child->detail, child->subkey, child->offset);
                child.swap(object);
                child->parent = parameter;
                child->deserialize(&jchild);
                continue;
            }
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

        for (auto& child : parameter->sorted_children()) {
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

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&jchild);
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

        for (auto& child : parameter->sorted_children()) {
            auto& jchild = jout[child->subkey];
            child->serialize(&jchild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& jin = (*static_cast<const nlohmann::ordered_json*>(in));

        parameter->children.clear();

        for (const auto& jpair : jin.items()) {
            const auto subkey = jpair.key();
            auto& jchild = jpair.value();
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&jchild);
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

        for (auto& child : parameter->sorted_children()) {
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

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& jchild : jin) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&jchild);
        }
    }
};

}