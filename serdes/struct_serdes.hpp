#pragma once

#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"
#include "serdes/validator/validator.hpp"

namespace xvorin::serdes {

template <typename T, typename E = void> //[T]ype & [E]nable
class StructSerdes : public Serdes {
};

// BASIC
template <typename T>
class StructSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
public:
    virtual void init(const Parameter& owner) override
    {
        validator.parse_verification_info(owner.verinfo);
    }

private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);

        if (!validator(parameter->value)) {
            parameter->profile()->violations[p->index()] = Converter<T>::to_string(parameter->value);
            return;
        }

        if (sout == parameter->value) {
            return;
        }

        if (!parameter->profile()->inform_enabled) {
            sout = parameter->value;
            return;
        }

        if (!parameter->inform) {
            sout = parameter->value;
            parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_CHANGE);
            return;
        }

        if (parameter->inform(parameter->index(), sout, parameter->value)) {
            sout = parameter->value;
            parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_CHANGE);
            return;
        }

        parameter->profile()->violations[p->index()] = Converter<T>::to_string(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        parameter->value = sin;
    }

    Validator<T> validator;
};

// ENUM
template <typename T>
class StructSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
public:
    virtual void init(const Parameter& owner) override
    {
        validator.parse_verification_info(owner.verinfo);
    }

private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);

        if (!validator(parameter->value)) {
            parameter->profile()->violations[p->index()] = Converter<T>::to_string(parameter->value);
            return;
        }

        if (sout == parameter->value) {
            return;
        }

        if (!parameter->profile()->inform_enabled) {
            sout = parameter->value;
            return;
        }

        if (!parameter->inform) {
            sout = parameter->value;
            parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_CHANGE);
            return;
        }

        if (parameter->inform(parameter->index(), sout, parameter->value)) {
            sout = parameter->value;
            parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_CHANGE);
            return;
        }

        parameter->profile()->violations[p->index()] = Converter<T>::to_string(parameter->value);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        parameter->value = sin;
    }

    Validator<T> validator;
};

// OBJECT
template <typename T>
class StructSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);

        for (const auto& child_pair : parameter->children()) {
            const auto& child = child_pair.second;
            auto schild = static_cast<char*>(out) + child->offset;
            child->serialize(schild);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);

        for (auto& child_pair : (*parameter->mutable_children())) {
            auto& child = child_pair.second;
            auto schild = static_cast<const char*>(in) + child->offset;
            child->deserialize(schild);
        }
    }
};

// SEQUENCE
template <typename T>
class StructSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);

        size_t counter = 0;
        for (auto& child : parameter->sorted_children()) {
            if (sout.size() <= counter) {
                sout.emplace_back();
                const auto premitive = parameter->profile()->inform_enabled;
                parameter->profile()->inform_enabled = false;
                child->serialize(&sout.back());
                parameter->profile()->inform_enabled = premitive;
                if (parameter->profile()->inform_enabled) {
                    const auto subkey = parameter->index() + "." + std::to_string(counter);
                    parameter->inform_ancestor(subkey, ParameterInformType::PIT_CREATE);
                }
            } else {
                auto iter = sout.begin();
                for (size_t i = 0; i < counter; i++) {
                    iter++;
                }
                child->serialize(&(*iter));
            }
            counter++;
        }

        while (sout.size() > counter) {
            sout.pop_back();
            if (parameter->profile()->inform_enabled) {
                const auto subkey = parameter->index() + "." + std::to_string(sout.size());
                parameter->inform_ancestor(subkey, ParameterInformType::PIT_REMOVE);
            }
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& schild : sin) {
            const auto subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&schild);
            children->emplace(subkey, child);
        }
    }
};

// MAP
template <typename T>
class StructSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);

        std::set<typename T::key_type> subkeys;
        for (const auto& child : parameter->children()) {
            auto subkey = Converter<typename T::key_type>::from_string(child.first);
            if (sout.find(subkey) == sout.end() && parameter->profile()->inform_enabled) {
                const auto premitive = parameter->profile()->inform_enabled;
                parameter->profile()->inform_enabled = false;
                child.second->serialize(&sout[subkey]);
                parameter->profile()->inform_enabled = premitive;
                parameter->inform_ancestor(child.second->index(), ParameterInformType::PIT_CREATE);
            } else {
                child.second->serialize(&sout[subkey]);
            }
            subkeys.insert(subkey);
        }

        for (auto iter = sout.begin(); iter != sout.end();) {
            if (subkeys.find(iter->first) == subkeys.end()) {
                const auto subkey = parameter->index() + "." + Converter<typename T::key_type>::to_string(iter->first);
                iter = sout.erase(iter);
                if (parameter->profile()->inform_enabled) {
                    parameter->inform_ancestor(subkey, ParameterInformType::PIT_REMOVE);
                }
            } else {
                iter++;
            }
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        auto children = parameter->mutable_children();
        children->clear();

        for (const auto& spair : sin) {
            const std::string subkey = Converter<typename T::key_type>::to_string(spair.first);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&spair.second);
            children->emplace(subkey, child);
        }
    }
};

// SET
template <typename T>
class StructSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);
        auto nout = T();

        for (auto& child : parameter->sorted_children()) {
            typename T::value_type schild;
            const auto premitive = parameter->profile()->inform_enabled;
            parameter->profile()->inform_enabled = false;
            child->serialize(&schild);
            parameter->profile()->inform_enabled = premitive;
            nout.insert(schild);
        }

        for (auto& v : sout) {
            if (nout.find(v) == nout.end() && parameter->profile()->inform_enabled) {
                parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_REMOVE);
            }
        }

        for (auto& v : nout) {
            if (sout.find(v) == sout.end() && parameter->profile()->inform_enabled) {
                parameter->inform_ancestor(parameter->index(), ParameterInformType::PIT_CREATE);
            }
        }

        sout.swap(nout);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        auto children = parameter->mutable_children();
        children->clear();

        size_t counter = 0;
        for (const auto& schild : sin) {
            const std::string subkey = std::to_string(counter++);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(&schild);
            children->emplace(subkey, child);
        }
    }
};

// PTR
template <typename T>
class StructSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& sout = *reinterpret_cast<T*>(out);

        if (!parameter->value && !sout) {
            return;
        }

        if (!parameter->value && sout) {
            sout = nullptr;
            return;
        }

        if (!sout) {
            sout = parameter->make_smart_ptr();
        }
        parameter->value->serialize(sout.get());
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& sin = *reinterpret_cast<const T*>(in);

        if (sin == nullptr) {
            parameter->value = nullptr;
            return;
        }

        if (parameter->value == nullptr) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
        }

        parameter->value->deserialize(sin.get());
    }
};

}