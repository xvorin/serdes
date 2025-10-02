#pragma once

#include "serdes/types/registrar.hpp"

#include <fstream>
#include <iostream>
#include <memory>

#include <stdio.h>

namespace xvorin::serdes {

template <typename T>
class ParameterTree {
public:
    ParameterTree(const std::string& root);

    /**
     * 获取数据模型
     */
    std::string root();

    /**
     *  model_的内容发生变化时须调用此接口, 将数据刷回value_: model_->value_
     */
    void commit_model_changes();

    /**
     *  value_的内容发生变化时须调用此接口, 将数据刷入model_: value_->model_
     */
    void commit_value_changes();

    /**
     * 按指定方式对参数进行序列化
     */
    void serialize(std::shared_ptr<const Parameter> p, void*, ParameterSerdesType);

    /**
     * 按指定方式对参数进行反序列化
     */
    void deserialize(std::shared_ptr<Parameter> p, const void*, ParameterSerdesType);

    /**
     * 获取参数结构体，为避免参数修改与外部读取冲突通过拷贝获取
     */
    T value();

    /**
     * 获取可修改的参数结构体，为避免参数修改与外部读取冲突，需使用 std::unique_lock<std::mutex> 锁进行访问
     */
    T& mutable_value(std::unique_lock<std::mutex>& holder);

    /**
     * 创建参数
     * **只支持针对STL对象执行创建操作;
     * **只支持新增一级参数,新增多级参数需多次调用
     * @param index 参数索引,形如"a.b.c.d", 针对vector/list/set等类型 索引值为整数值. 下同
     */
    void create(const std::string& index);

    /**
     * 删除参数,只支持针对STL对象执行删除操作
     */
    void remove(const std::string& index);

    /**
     * 获取参数,只支持针对基础类型进行读取;读取需指定参数类型
     */
    template <typename V>
    V get(const std::string& index);

    /**
     * 设置参数,只支持针对基础类型进行设置
     */
    template <typename V>
    void set(const std::string& index, const V& value);

    /**
     * 设置参数,对std::string类型的特化
     */
    void set(const std::string& index, const std::string& value);

    /**
     * 获取参数内部存储结构(具体类型),参数不合法或不存在时抛出异常
     */
    template <typename V>
    std::shared_ptr<const TraitedParameter<V>> parameter(const std::string& index);

    /**
     * 获取参数内部存储结构(抽象类型),参数不合法或不存在时抛出异常
     */
    std::shared_ptr<Parameter> parameter(const std::string& index);

    /**
     * 探测参数是否存在,参数不合法或不存在时返回nullptr
     */
    std::shared_ptr<Parameter> exist(const std::string& index);

    /**
     * 使能/关闭变更通知
     */
    void enable_inform(bool enable = true);

    /**
     * for debug
     */
    std::string debug_string(const std::string& index = "");

private:
    std::list<std::string> parse_index(const std::string& index);
    std::shared_ptr<Parameter> parameter(std::list<std::string> index);

private:
    T value_;
    std::shared_ptr<Parameter> model_;
    std::mutex lock_;

private:
    ParameterTree(const ParameterTree&) = delete;
    ParameterTree& operator=(const ParameterTree&) = delete;

    ParameterTree(const ParameterTree&&) = delete;
    ParameterTree& operator=(const ParameterTree&&) = delete;
};

template <typename T>
ParameterTree<T>::ParameterTree(const std::string& root)
{
    // 构造model
    auto model = ParameterPrototype::create_parameter(typeid(T), root);
    if (!model) {
        throw std::runtime_error(std::string("No Parameter prototype for ") + Parameter::readable_detail_type(typeid(T)));
    }
    model->init_profile();
    model_ = model;

    // 合法性自检
    try {
        commit_value_changes();
        commit_model_changes();
    } catch (const std::exception& e) {
        std::cerr << "Default parameter value fail the legality check!" << e.what() << std::endl;
        exit(-1);
    }
}

template <typename T>
std::string ParameterTree<T>::root()
{
    return model_->subkey;
}

template <typename T>
void ParameterTree<T>::commit_model_changes()
{
    serialize(model_, &value_, ParameterSerdesType::PST_STRU);
}

template <typename T>
void ParameterTree<T>::commit_value_changes()
{
    deserialize(model_, &value_, ParameterSerdesType::PST_STRU);
}

template <typename T>
void ParameterTree<T>::serialize(std::shared_ptr<const Parameter> p, void* ptr, ParameterSerdesType type)
{
    std::unordered_map<std::string, std::string> violations;
    {
        std::lock_guard<std::mutex> guard(lock_);
        model_->profile()->serdes_type = type;
        model_->profile()->violations.clear();
        p->serialize(ptr);
        violations.swap(model_->profile()->violations);
    }

    std::unordered_map<std::string, std::pair<std::string, std::string>> exceptions;
    for (auto& violation : violations) {
        const auto index = violation.first;
        const auto info = parameter(parse_index(index))->debug_releation();
        exceptions[index] = std::make_pair(violation.second, info);
    }

    if (!exceptions.empty()) {
        throw ViolationException(exceptions);
    }
}

template <typename T>
void ParameterTree<T>::deserialize(std::shared_ptr<Parameter> p, const void* ptr, ParameterSerdesType type)
{
    std::lock_guard<std::mutex> guard(lock_);
    model_->profile()->serdes_type = type;
    p->deserialize(ptr);
}

template <typename T>
T ParameterTree<T>::value()
{
    std::lock_guard<std::mutex> guard(lock_);
    return value_;
}

template <typename T>
T& ParameterTree<T>::mutable_value(std::unique_lock<std::mutex>& holder)
{
    std::unique_lock<std::mutex> guard(lock_);
    holder.swap(guard);
    return value_;
}

template <typename T>
void ParameterTree<T>::create(const std::string& index)
{
    auto parsed_index = parse_index(index);
    if (parsed_index.size() < 2) {
        throw InvalidIndex(index);
    }

    auto subkey = parsed_index.back();
    parsed_index.pop_back();

    {
        std::lock_guard<std::mutex> guard(lock_);
        auto retval = parameter(parsed_index);
        if (nullptr == retval) {
            throw ParameterNotFound(parsed_index);
        }
        retval->create_child(subkey);
    }

    commit_model_changes();
}

template <typename T>
void ParameterTree<T>::remove(const std::string& index)
{
    auto parsed_index = parse_index(index);
    if (parsed_index.size() < 2) {
        throw InvalidIndex(index);
    }

    auto subkey = parsed_index.back();
    parsed_index.pop_back();

    {
        std::lock_guard<std::mutex> guard(lock_);
        auto retval = parameter(parsed_index);
        if (nullptr == retval) {
            throw ParameterNotFound(parsed_index);
        }
        retval->remove_child(subkey);
    }

    commit_model_changes();
}

template <typename T>
template <typename V>
V ParameterTree<T>::get(const std::string& index)
{
    static_assert(is_basic<V>::value, "get<>() only support basic type");
    return static_cast<V>(parameter<V>(index)->value);
}

template <typename T>
template <typename V>
void ParameterTree<T>::set(const std::string& index, const V& value)
{
    static_assert(is_basic<V>::value, "set<>() only support basic type");
    const_cast<V&>(parameter<V>(index)->value) = value;
}

template <typename T>
void ParameterTree<T>::set(const std::string& index, const std::string& value)
{
    parameter(index)->from_string(value);
}

template <typename T>
template <typename V>
std::shared_ptr<const TraitedParameter<V>> ParameterTree<T>::parameter(const std::string& index)
{
    return std::dynamic_pointer_cast<TraitedParameter<V>>(parameter(index));
}

template <typename T>
std::shared_ptr<Parameter> ParameterTree<T>::parameter(const std::string& index)
{
    std::lock_guard<std::mutex> guard(lock_);
    auto retval = parameter(parse_index(index));
    if (nullptr == retval) {
        throw ParameterNotFound(index);
    }
    return retval;
}

template <typename T>
std::shared_ptr<Parameter> ParameterTree<T>::exist(const std::string& index)
{
    std::lock_guard<std::mutex> guard(lock_);
    return parameter(parse_index(index));
}

template <typename T>
std::string ParameterTree<T>::debug_string(const std::string& index)
{
    commit_value_changes();

    std::lock_guard<std::mutex> guard(lock_);
    auto p = model_;
    if (!index.empty()) {
        p = parameter(parse_index(index));
    }

    if (p == nullptr) {
        throw ParameterNotFound(index);
    }

    std::stringstream ss;
    ss << p->debug_releation();
    return ss.str();
}

template <typename T>
void ParameterTree<T>::enable_inform(bool enable)
{
    model_->profile()->inform_enabled = enable;
}

template <typename T>
std::list<std::string> ParameterTree<T>::parse_index(const std::string& index)
{
    std::list<std::string> retval;
    const std::string delim = ".";

    std::string::const_iterator substart = index.begin();
    std::string::const_iterator subend = substart;
    while (true) {
        subend = std::search(substart, index.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);
        if (!temp.empty()) {
            retval.push_back(temp);
        }
        if (subend == index.end()) {
            break;
        }
        substart = subend + delim.size();
    }

    if (retval.empty()) {
        throw InvalidIndex(index);
    }

    return retval;
}

template <typename T>
std::shared_ptr<Parameter> ParameterTree<T>::parameter(std::list<std::string> index)
{
    if (index.empty() || model_->subkey != index.front()) {
        return nullptr;
    }

    index.pop_front(); // pop root
    std::shared_ptr<Parameter> retval = model_;

    while (!index.empty()) {
        const auto curr = index.front();
        index.pop_front();

        retval = retval->find_child(curr);
        if (nullptr == retval) {
            break;
        }
    }
    return index.empty() ? retval : nullptr;
}

}