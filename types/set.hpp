#pragma once

#include "serdes/types/composite.hpp"
#include "serdes/types/traits.hpp"

namespace xvorin::serdes {

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_set<T>::value>::type> : public Composite<T> {
    using subtype = typename T::value_type;

    TraitedParameter(std::string subkey, size_t offset, std::string comment,
        typename Composite<T>::inform_type inform, std::string verinfo)
        : Composite<T>(std::move(subkey), typeid(subtype), offset, std::move(comment), std::move(inform), std::move(verinfo))
    {
        this->sort_children_strategy = sort_parameter_by_number;
    }

    virtual void create_child(const std::string& newkey) override
    {
        size_t counter = 0;
        decltype(this->children) nchildren;

        for (auto& child : this->sorted_children()) {
            auto subkey = std::to_string(counter++);
            if (subkey == newkey) {
                nchildren[subkey] = ParameterPrototype::create_parameter(this->detail, subkey);
                nchildren[subkey]->parent = this->shared_from_this();
                subkey = std::to_string(counter++);
            }
            nchildren[subkey] = child;
            const_cast<std::string&>(child->subkey) = subkey;
        }

        if (counter == this->children.size()) {
            const auto subkey = std::to_string(counter++);
            nchildren[subkey] = ParameterPrototype::create_parameter(this->detail, subkey);
            nchildren[subkey]->parent = this->shared_from_this();
        }

        this->children.swap(nchildren);
    }

    virtual void remove_child(const std::string& subkey) override
    {
        auto iter = this->children.find(subkey);
        if (iter == this->children.end()) {
            throw ParameterNotFound(this->index() + "." + subkey);
        }
        this->children.erase(iter);

        // 为节点重新排序
        size_t counter = 0;
        decltype(this->children) nchildren;
        for (auto& child : this->sorted_children()) {
            const auto subkey = std::to_string(counter++);
            nchildren[subkey] = child;
            const_cast<std::string&>(child->subkey) = subkey;
        }

        this->children.swap(nchildren);
    }
};

}