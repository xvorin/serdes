#pragma once

#include "serdes/serdes/multi_serdes.hpp"
#include "serdes/types/composite.hpp"

namespace xvorin::serdes {

namespace {

template <typename T>
ParameterType trait_type()
{
    if (is_basic<T>::value) {
        return ParameterType::PT_BASIC;
    }
    if (is_enum<T>::value) {
        return ParameterType::PT_ENUM;
    }
    if (is_object<T>::value) {
        return ParameterType::PT_OBJECT;
    }
    if (is_sequence<T>::value) {
        return ParameterType::PT_SEQUENCE;
    }
    if (is_map<T>::value) {
        return ParameterType::PT_MAP;
    }
    if (is_set<T>::value) {
        return ParameterType::PT_SET;
    }
    throw TypeNotSupport(Parameter::readable_detail_type(typeid(T)));
}

}

template <typename T>
Composite<T>::Composite(std::string subkey, ParameterDetailType detail, size_t offset,
    std::string comment, inform_type inform, std::string verinfo)
    : Parameter(std::move(subkey), trait_type<T>(), detail, offset, std::move(comment), std::move(verinfo))
    , inform(std::move(inform))
{
    serdes_ = std::make_shared<MultiSerdes<T>>(*this);
}

template <typename T>
std::shared_ptr<Parameter> Composite<T>::clone(std::string newkey, size_t newoffset) const
{
    auto retval = std::make_shared<TraitedParameter<T>>(std::move(newkey), newoffset, comment, inform, verinfo);
    retval->parent = parent;

    for (auto& child : children) {
        auto retval_child = child.second->clone(child.second->subkey, child.second->offset);
        retval_child->parent = retval;
        retval->children[child.first] = retval_child;
    }
    return retval;
}

template <typename T>
void Composite<T>::serialize(void* out) const
{
    serdes_->serialize(shared_from_this(), out);
}

template <typename T>
void Composite<T>::deserialize(const void* in)
{
    serdes_->deserialize(shared_from_this(), in);
}

template <typename T>
std::string Composite<T>::debug_self() const
{
    std::stringstream ss;
    ss << " size = " << children.size();
    if (!comment.empty()) {
        ss << " #" << comment;
    }
    // if (!verinfo.empty()) {
    //     ss << " " << ";校验值:" << verinfo;
    // }
    return ss.str();

    // std::stringstream ss;
    // ss << " size = " << children.size() << " ";
    // ss << "[类型:" << readable_type() << "/" << readable_detail_type();
    // ss << ";偏移:" << offset;
    // if (!comment.empty()) {
    //     ss << ";注释:" << comment;
    // }
    // if (!verinfo.empty()) {
    //     ss << ";校验值:" << verinfo;
    // }
    // ss << (is_stl<T>::value ? ";STL类型:" + Parameter::readable_detail_type(typeid(T)) + "]" : "]");

    // return ss.str();
}

template <typename T>
std::string Composite<T>::debug_releation(const std::string& prefix) const
{
    std::stringstream ss;
    ss << prefix << "|" << index() << debug_self();

    // auto p = parent.lock();
    // if (p) {
    //     ss << std::endl
    //        << prefix << "    |PARENT:" << (p ? p->debug_self() : "null");
    // }

    // ss << std::endl
    //    << prefix << "    |CHILDREN:" << children.size();

    for (auto& child : sorted_children()) {
        ss << std::endl
           << child->debug_releation(prefix + "  ");
    }

    return ss.str();
}

template <typename T>
std::shared_ptr<Parameter> Composite<T>::find_child(const std::string& subkey) const
{
    auto iter = children.find(subkey);
    return (iter == children.end()) ? nullptr : iter->second;
}

template <typename T>
size_t Composite<T>::children_size() const
{
    return children.size();
}

template <typename T>
std::unordered_map<std::string, std::shared_ptr<Parameter>> Composite<T>::get_children() const
{
    return children;
}

template <typename T>
void Composite<T>::inform_ancestor(const std::string& index, ParameterInformType pit) const
{
    if (inform) {
        inform(index, pit);
    }
    auto p = parent.lock();
    if (p) {
        p->inform_ancestor(index, pit);
    }
}

template <typename T>
void Composite<T>::insert_child(std::shared_ptr<Parameter> param)
{
    if (!param) {
        return;
    }
    children[param->subkey] = param;
    param->parent = shared_from_this();
}

template <typename T>
std::list<std::shared_ptr<Parameter>> Composite<T>::sorted_children(SortCompareCallback cb) const
{
    if (!cb) {
        cb = sort_children_strategy;
    }

    if (!cb) {
        cb = sort_parameter_by_offset;
    }

    std::list<std::shared_ptr<Parameter>> sorted;
    for (auto& child : children) {
        sorted.push_back(child.second);
    }

    sorted.sort(cb);
    return sorted;
}

}
