#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <yaml-cpp/yaml.h>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class YamlSerdes : public Serdes {
};

// BASIC
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));

        yout = YAML::Node(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));

        parameter->value = yin.as<T>();
    }
};

// ENUM
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));

        yout = YAML::Node(static_cast<int>(parameter->value));
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));

        parameter->value = static_cast<T>(yin.as<int>());
    }
};

// OBJECT
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));

        for (auto& child : parameter->sorted_children()) {
            YAML::Node ychild = yout[child->subkey];
            child->serialize(&ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));

        for (auto& cpair : parameter->children) {
            auto& child = cpair.second;

            auto ychild = yin[child->subkey];
            if (!ychild.IsDefined()) {
                continue;
            }

            if (child->type == ParameterType::PT_OBJECT) {
                auto object = ParameterPrototype::create_parameter(child->detail, child->subkey, child->offset);
                child.swap(object);
                child->parent = parameter;
                child->deserialize(&ychild);
                continue;
            }
            child->deserialize(&ychild);
        }
    }
};

// SEQUENCE
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));
        yout = YAML::Node(YAML::NodeType::Sequence);

        for (auto& child : parameter->sorted_children()) {
            YAML::Node ychild;
            child->serialize(&ychild);
            yout.push_back(ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));
        if (!yin.IsSequence()) {
            return;
        }

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& ychild : yin) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&ychild);
        }
    }
};

// MAP
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));
        yout = YAML::Node(YAML::NodeType::Map);

        for (auto& child : parameter->sorted_children()) {
            auto ychild = yout[child->subkey];
            child->serialize(&ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));
        if (!yin.IsMap()) {
            return;
        }

        parameter->children.clear();

        for (const auto& ypair : yin) {
            const auto subkey = ypair.first.as<std::string>();
            const auto ychild = YAML::Node(ypair.second);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&ychild);
        }
    }
};

// SET
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& yout = (*static_cast<YAML::Node*>(out));
        yout = YAML::Node(YAML::NodeType::Sequence);

        for (auto& child : parameter->sorted_children()) {
            YAML::Node ychild;
            child->serialize(&ychild);
            yout.push_back(ychild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& yin = (*static_cast<const YAML::Node*>(in));
        if (!yin.IsSequence()) {
            return;
        }

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& ychild : yin) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&ychild);
        }
    }
};

}