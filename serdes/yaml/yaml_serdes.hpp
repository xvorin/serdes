#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <serdes/3rd/fkYAML/node.hpp>

namespace fkyaml {
using ordered_node = basic_node<std::vector, fkyaml::ordered_map>;
}

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

// OBJECT
template <typename T>
class YamlSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
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

        for (const auto& child_pair : parameter->children()) {
            auto& child = child_pair.second;

            if (!yin.contains(child->subkey)) {
                continue;
            }

            const auto& ychild = yin[child->subkey];
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

}