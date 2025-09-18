#pragma once

#include "serdes/serdes/multi_serdes.hpp"
#include "serdes/types/basic.hpp"

#include "serdes/types/map_key.hpp"

namespace xvorin::serdes {

template <typename T>
BasicParameter<T>::TraitedParameter(std::string subkey, size_t offset, std::string comment, inform_type inform, std::string verinfo)
    : Parameter(std::move(subkey), ParameterType::PT_BASIC, typeid(T), offset, std::move(comment), std::move(verinfo))
    , inform(std::move(inform))
{
    serdes_ = std::make_shared<MultiSerdes<T>>(*this);
}

template <typename T>
std::shared_ptr<Parameter> BasicParameter<T>::clone(std::string newkey, size_t newoffset) const
{
    auto retval = std::make_shared<BasicParameter<T>>(std::move(newkey), newoffset, comment, inform, verinfo);
    retval->parent = parent;
    retval->value = value;
    return retval;
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
    value = xvorin::serdes::from_string<T>(in);
}

template <typename T>
std::string BasicParameter<T>::debug_self() const
{
    std::stringstream ss;
    ss << " = " << xvorin::serdes::to_string(value);
    if (!comment.empty()) {
        ss << " #" << comment;
    }
    // if (!verinfo.empty()) {
    //     ss << " " << ";校验值:" << verinfo;
    // }
    return ss.str();

    // std::stringstream ss;
    // ss << " = " << xvorin::serdes::to_string(value) << " ";
    // ss << "[类型:" << readable_type() << "/" << readable_detail_type();
    // ss << ";偏移:" << offset;
    // if (!comment.empty()) {
    //     ss << ";注释:" << comment;
    // }
    // if (!verinfo.empty()) {
    //     ss << ";校验值:" << verinfo;
    // }
    // ss << "]";
    // return ss.str();
}

template <typename T>
std::string BasicParameter<T>::debug_releation(const std::string& prefix) const
{
    std::stringstream ss;
    ss << prefix << "|" << index() << debug_self();

    // auto p = parent.lock();
    // if (p) {
    //     ss << std::endl
    //        << prefix << "    |PARENT:" << (p ? p->debug_self() : "null");
    // }
    return ss.str();
};

template <typename T>
void BasicParameter<T>::inform_ancestor(const std::string& index, ParameterInformType pit) const
{
    auto p = parent.lock();
    if (p) {
        p->inform_ancestor(index, pit);
    }
}

}