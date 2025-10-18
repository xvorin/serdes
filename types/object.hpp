#pragma once

#include "serdes/types/composite.hpp"
#include "serdes/types/traits.hpp"

namespace xvorin::serdes {

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_object<T>::value>::type> : public Composite<T> {
    using subtype = T;

    TraitedParameter(std::string subkey, size_t offset, std::string comment,
        typename Composite<T>::informant inform, std::string verinfo)
        : Composite<T>(std::move(subkey), typeid(subtype), offset, std::move(comment), std::move(inform), std::move(verinfo))
    {
        this->sort_children_strategy = sort_parameter_by_offset;
    }

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override
    {
        auto cloned = std::make_shared<TraitedParameter<T>>(std::move(newkey), newoffset, this->comment, this->inform, this->verinfo);
        cloned->parent = this->parent;
        cloned->bases = this->bases;

        // 添加自身的成员
        for (const auto& child : this->children()) {
            auto cloned_child = child.second->clone(child.second->subkey, child.second->offset);
            cloned_child->parent = cloned;
            cloned->mutable_children()->emplace(child.first, cloned_child);
        }

        // 添加基类的成员
        for (auto& base : this->bases) {
            auto base_parameter = ParameterPrototype::create_parameter(base, "temporary");
            for (const auto& child : base_parameter->children()) {
                auto cloned_child = child.second->clone(child.second->subkey, child.second->offset);
                cloned_child->parent = cloned;
                cloned->mutable_children()->emplace(child.first, cloned_child);
            }
        }

        return cloned;
    }

    std::string debug_self() const override
    {
        std::stringstream ss;
        ss << " size = " << this->children().size() << " [" + this->readable_detail_type();
        if (!this->bases.empty()) {
            ss << ":";
            for (auto iter = this->bases.begin(); iter != this->bases.end(); iter++) {
                ss << (iter == this->bases.begin() ? "" : ",") << Parameter::readable_detail_type(*iter);
            }
        }
        ss << "]";

        if (!this->comment.empty()) {
            ss << " #" << this->comment;
        }
        return ss.str();
    }

    using BaseClasses = std::unordered_set<ParameterDetailType, ParameterDetailTypeHash, ParameterDetailTypeEqual>;

    BaseClasses bases;
};

}