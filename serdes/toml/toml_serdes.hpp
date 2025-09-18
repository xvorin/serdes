#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <toml11/toml.hpp>

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class TomlSerdes : public Serdes {
};

// BASIC
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = parameter->value;

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    template <typename V>
    void from_toml_value(V& out, const toml::value& in)
    {
        out = in.as_integer();
    }

    void from_toml_value(std::string& out, const toml::value& in)
    {
        out = in.as_string();
    }

    void from_toml_value(bool& out, const toml::value& in)
    {
        out = in.as_boolean();
    }

    void from_toml_value(float& out, const toml::value& in)
    {
        out = in.as_floating();
    }

    void from_toml_value(double& out, const toml::value& in)
    {
        out = in.as_floating();
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const toml::value*>(in));
        from_toml_value(parameter->value, tin);
    }
};

// ENUM
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = static_cast<int>(parameter->value);

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const toml::value*>(in));
        parameter->value = static_cast<T>(tin.as_integer());
    }
};

// OBJECT
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = toml::table();

        auto children = parameter->sorted_children();
        std::reverse(children.begin(), children.end());
        for (auto& child : children) {
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
        auto& tin = (*static_cast<const toml::value*>(in));
        if (!tin.is_table()) {
            return;
        }

        for (auto& cpair : parameter->children) {
            auto& child = cpair.second;

            if (tin.as_table().find(child->subkey) == tin.as_table().end()) {
                continue;
            }

            const auto& tchild = tin.at(child->subkey);

            if (child->type == ParameterType::PT_OBJECT) {
                auto object = ParameterPrototype::create_parameter(child->detail, child->subkey, child->offset);
                child.swap(object);
                child->parent = parameter;
                child->deserialize(&tchild);
                continue;
            }
            child->deserialize(&tchild);
        }
    }
};

// SEQUENCE
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = toml::array();

        for (auto& child : parameter->sorted_children()) {
            toml::value tchild;
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
        auto& tin = (*static_cast<const toml::value*>(in));
        if (!tin.is_array()) {
            return;
        }

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&tchild);
        }
    }
};

// MAP
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = toml::table();

        auto children = parameter->sorted_children();
        std::reverse(children.begin(), children.end());
        for (auto& child : children) {
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
        auto& tin = (*static_cast<const toml::value*>(in));
        if (!tin.is_table()) {
            return;
        }

        parameter->children.clear();

        for (const auto& tpair : tin.as_table()) {
            const auto subkey = to_string(tpair.first);
            const auto tchild = tpair.second;

            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&tchild);
        }
    }
};

// SET
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::value*>(out));
        tout = toml::array();

        for (auto& child : parameter->sorted_children()) {
            toml::value tchild;
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
        auto& tin = (*static_cast<const toml::value*>(in));
        if (!tin.is_array()) {
            return;
        }

        parameter->children.clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(&tchild);
        }
    }
};

}