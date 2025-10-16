#pragma once

#include "serdes/serdes/multi_serdes.hpp"
#include "serdes/types/enum.hpp"

namespace xvorin::serdes {

template <typename T>
EnumParameter<T>::TraitedParameter(std::string subkey, size_t offset,
    std::string comment, informant inform, std::string verinfo)
    : Parameter(std::move(subkey), ParameterType::PT_ENUM, typeid(T), offset, std::move(comment), std::move(verinfo))
    , inform(std::move(inform))
{
    serdes_ = std::make_shared<MultiSerdes<T>>(*this);
}

template <typename T>
std::shared_ptr<Parameter> EnumParameter<T>::clone(std::string newkey, size_t newoffset) const
{
    auto cloned = std::make_shared<EnumParameter<T>>(std::move(newkey), newoffset, comment, inform, verinfo);
    cloned->parent = parent;
    cloned->value = value;
    cloned->enum_mapping = enum_mapping;

    return cloned;
}

template <typename T>
void EnumParameter<T>::serialize(void* out) const
{
    serdes_->serialize(shared_from_this(), out);
}

template <typename T>
void EnumParameter<T>::deserialize(const void* in)
{
    serdes_->deserialize(shared_from_this(), in);
}

template <typename T>
void EnumParameter<T>::from_string(const std::string& in)
{
    value = Converter<T>::from_string(in);
}

template <typename T>
void EnumParameter<T>::inform_ancestor(const std::string& index, ParameterInformType pit) const
{
    auto p = parent.lock();
    if (p) {
        p->inform_ancestor(index, pit);
    }
}

template <typename T>
std::string EnumParameter<T>::debug_self() const
{
    std::stringstream ss;
    ss << " = " << Converter<T>::to_string(value) << "(" << static_cast<int>(value) << ")";
    ss << " [" + this->readable_detail_type();
    if (!enum_mapping.empty()) {
        ss << "(";
        for (auto iter = enum_mapping.begin(); iter != enum_mapping.end(); iter++) {
            ss << (iter != enum_mapping.begin() ? "," : "") << iter->second;
        }
        ss << ")";
    }
    ss << "]";

    if (!comment.empty()) {
        ss << " #" << comment;
    }
    return ss.str();
}

template <typename T>
std::string EnumParameter<T>::debug_releation(const std::string& prefix) const
{
    std::stringstream ss;
    ss << prefix << "|" << index() << debug_self();
    return ss.str();
}

}