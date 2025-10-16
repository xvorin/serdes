#pragma once

#include "serdes/serdes/multi_serdes.hpp"
#include "serdes/types/basic.hpp"

#include "serdes/utils/converter.h"

namespace xvorin::serdes {

template <typename T>
BasicParameter<T>::TraitedParameter(std::string subkey, size_t offset,
    std::string comment, informant inform, std::string verinfo)
    : Parameter(std::move(subkey), ParameterType::PT_BASIC, typeid(T), offset, std::move(comment), std::move(verinfo))
    , inform(std::move(inform))
{
    serdes_ = std::make_shared<MultiSerdes<T>>(*this);
}

template <typename T>
std::shared_ptr<Parameter> BasicParameter<T>::clone(std::string newkey, size_t newoffset) const
{
    auto cloned = std::make_shared<BasicParameter<T>>(std::move(newkey), newoffset, comment, inform, verinfo);
    cloned->parent = parent;
    cloned->value = value;
    return cloned;
}

template <typename T>
void BasicParameter<T>::serialize(void* out) const
{
    serdes_->serialize(shared_from_this(), out);
}

template <typename T>
void BasicParameter<T>::deserialize(const void* in)
{
    serdes_->deserialize(shared_from_this(), in);
}

template <typename T>
void BasicParameter<T>::from_string(const std::string& in)
{
    value = Converter<T>::from_string(in);
}

template <typename T>
void BasicParameter<T>::inform_ancestor(const std::string& index, ParameterInformType pit) const
{
    auto p = parent.lock();
    if (p) {
        p->inform_ancestor(index, pit);
    }
}

template <typename T>
std::string BasicParameter<T>::debug_self() const
{
    std::stringstream ss;
    ss << " = " << Converter<T>::to_string(value) << " [" + this->readable_detail_type() + "]";
    if (!comment.empty()) {
        ss << " #" << comment;
    }
    return ss.str();
}

template <typename T>
std::string BasicParameter<T>::debug_releation(const std::string& prefix) const
{
    std::stringstream ss;
    ss << prefix << "|" << index() << debug_self();
    return ss.str();
}

}