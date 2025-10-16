#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <serdes/3rd/toml11/toml.hpp>

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
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = parameter->value;

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    template <typename V>
    void from_toml_value(V& out, const toml::ordered_value& in)
    {
        out = in.as_integer();
    }

    void from_toml_value(std::string& out, const toml::ordered_value& in)
    {
        out = in.as_string();
    }

    void from_toml_value(bool& out, const toml::ordered_value& in)
    {
        out = in.as_boolean();
    }

    void from_toml_value(float& out, const toml::ordered_value& in)
    {
        out = in.as_floating();
    }

    void from_toml_value(double& out, const toml::ordered_value& in)
    {
        out = in.as_floating();
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        from_toml_value(parameter->value, tin);
    }
};

// ENUM
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = Converter<T>::to_string(parameter->value);

        if (!parameter->comment.empty()) {
            tout.comments().push_back(parameter->comment);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        parameter->value = Converter<T>::from_string(tin.as_string());
    }
};

// OBJECT
template <typename T>
class TomlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = toml::ordered_table();

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
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        if (!tin.is_table()) {
            return;
        }

        for (const auto& child_pair : parameter->children()) {
            const auto& child = child_pair.second;

            if (!tin.contains(child->subkey)) {
                continue;
            }

            const auto& tchild = tin.at(child->subkey);
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
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = toml::ordered_array();

        for (const auto& child : parameter->sorted_children()) {
            toml::ordered_value tchild;
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
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        if (!tin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = toml::ordered_table();

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
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        if (!tin.is_table()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        for (const auto& tpair : tin.as_table()) {
            const auto& subkey = tpair.first;
            const auto& tchild = tpair.second;
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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
        auto& tout = (*static_cast<toml::ordered_value*>(out));
        tout = toml::ordered_array();

        for (const auto& child : parameter->sorted_children()) {
            toml::ordered_value tchild;
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
        auto& tin = (*static_cast<const toml::ordered_value*>(in));
        if (!tin.is_array()) {
            return;
        }

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& tchild : tin.as_array()) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey);
            child->parent = parameter;
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
        auto& tout = (*static_cast<toml::ordered_value*>(out));

        if (parameter->value) {
            parameter->value->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& tin = (*static_cast<const toml::ordered_value*>(in));

        if (!tin.is_empty()) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0");
            parameter->value->parent = parameter;
            parameter->value->deserialize(in);
        }
    }
};

}