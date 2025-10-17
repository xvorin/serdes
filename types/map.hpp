#pragma once

#include "serdes/types/composite.hpp"
#include "serdes/types/traits.hpp"

#include "serdes/utils/converter.hpp"

namespace xvorin::serdes {

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_map<T>::value>::type> : public Composite<T> {
    using subtype = typename T::mapped_type;

    TraitedParameter(std::string subkey, size_t offset, std::string comment,
        typename Composite<T>::informant inform, std::string verinfo)
        : Composite<T>(std::move(subkey), typeid(subtype), offset, std::move(comment), std::move(inform), std::move(verinfo))
    {
        this->sort_children_strategy = sort_parameter_by_suffix_number;
    }

    virtual void create_child(const std::string& newkey) override
    {
        auto iter = this->children().find(newkey);
        if (iter != this->children().end()) {
            throw IndexDuplicate(this->index() + "." + newkey);
        }

        auto child = ParameterPrototype::create_parameter(this->detail, newkey);
        child->parent = this->shared_from_this();
        this->mutable_children()->emplace(newkey, child);
    }

    virtual void remove_child(const std::string& subkey) override
    {
        auto iter = this->children().find(subkey);
        if (iter == this->children().end()) {
            throw ParameterNotFound(this->index() + "." + subkey);
        }

        this->mutable_children()->erase(iter);
    }
};

}