#pragma once

#include "serdes/types/composite.hpp"
#include "serdes/types/traits.hpp"

namespace xvorin::serdes {

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_object<T>::value>::type> : public Composite<T> {
    using subtype = T;

    TraitedParameter(std::string subkey, size_t offset, std::string comment,
        typename Composite<T>::inform_type inform, std::string verinfo)
        : Composite<T>(std::move(subkey), typeid(subtype), offset, std::move(comment), std::move(inform), std::move(verinfo))
    {
        this->sort_children_strategy = sort_parameter_by_offset;
    }

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override
    {
        auto retval = std::make_shared<TraitedParameter<T>>(std::move(newkey), newoffset, this->comment, this->inform, this->verinfo);
        retval->parent = this->parent;
        retval->bases = this->bases;

        for (auto& child : this->children) {
            auto retval_child = child.second->clone(child.second->subkey, child.second->offset);
            retval_child->parent = retval;
            retval->children[child.first] = retval_child;
        }

        for (auto& base : this->bases) {
            for (auto& child : xvorin::serdes::ParameterPrototype::create_parameter(base, "temporary")->get_children()) {
                auto retval_child = child.second->clone(child.second->subkey, child.second->offset);
                retval_child->parent = retval;
                retval->children[child.first] = retval_child;
            }
        }

        return retval;
    }

    using TypeBases = std::unordered_set<ParameterDetailType, ParameterDetailTypeHash, ParameterDetailTypeEqual>;

    TypeBases bases;
};

}