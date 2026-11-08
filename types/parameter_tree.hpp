#pragma once

#include "serdes/types/registrar.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <stdio.h>

namespace xvorin::serdes {

template <typename U>
class CommandLineAPI;

template <typename T>
class ParameterTree {
public:
    explicit ParameterTree(const std::string& root);

    /**
     * 获取数据模型
     */
    std::string root();

    /**
     *  model_的内容发生变化时须调用此接口, 将数据刷回value_: model_->value_
     */
    void commit_model_changes();

    /**
     * 将 model_ 的内容刷入外部结构体 value: model_ → value
     */
    void commit_model_changes(T* value);

    /**
     *  value_的内容发生变化时须调用此接口, 将数据刷入model_: value_->model_
     */
    void commit_value_changes();

    /**
     * 将外部结构体内容刷入 model_: value → model_
     */
    void commit_value_changes(const T& value);

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
    T* mutable_value(std::unique_lock<std::mutex>& holder);

    /**
     * 非线程安全接口, 不可与create/remove/get/set等接口并行调用
     */
    T* unsafe_value();

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
    void set(const std::string& index, const char* value);

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
     * 使能/关闭 subtree 内联输出: true 时 to_xxx 将 subtree 内容以对象形式输出(替代路径字符串);
     * 仅影响 json/yaml/toml/xml 文本后端, protobuf 恒输出路径字符串
     */
    void enable_subtree_inline(bool enable = true);

    /**
     * 当前 subtree 内联输出开关(序列化闸门内读取, 供 protobuf 预生成 schema 用)
     */
    bool subtree_inline() const;

    /**
     * 线程安全的 subtree 重载入口(由文件监控回调调用): 树锁内按节点当前 path 重载内容,
     * 再同步回 value_ 缓存; type 为无扩展名子文件时的回退格式
     */
    void reload_subtree(const std::string& index, ParameterSerdesType type);

    /**
     * 树内全部 subtree 节点的索引(树锁内遍历, 供批量开启/关闭监控)
     */
    std::vector<std::string> subtree_indices();

    /**
     * 在树锁保护下访问 subtree 节点(读取 path/注入载体等), 非 subtree 或不存在返回 false
     */
    bool visit_subtree(const std::string& index, const std::function<void(Subtree&)>& visitor);

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
    std::mutex lock_; // 数据锁: 保护 model_/value_/profile
    mutable std::recursive_mutex serialize_lock_; // 序列化闸门: serialize/deserialize 互斥, 并保护 subtree_sink 标志窗口

protected:
    /// @brief 获取序列化闸门(供 CommandLineAPI::save 持闸门设置 subtree_sink); 同线程可重入
    std::unique_lock<std::recursive_mutex> lock()
    {
        return std::unique_lock<std::recursive_mutex>(serialize_lock_);
    }
    friend class CommandLineAPI<T>;

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
    auto model = ParameterPrototype::create_parameter(typeid(T), root, nullptr);
    if (!model) {
        throw std::runtime_error(std::string("No Parameter prototype for ") + Parameter::readable_detail_type(typeid(T)));
    }
    model->init_profile();
    model_ = model;

    // 合法性自检: 默认值不满足校验(verinfo)时抛异常(如 ViolationException), 由调用方决定终止/降级;
    // 不做 exit, 与库内异常习惯一致, 避免跳过析构与不可恢复的进程退出
    commit_value_changes();
    commit_model_changes();
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
void ParameterTree<T>::commit_model_changes(T* value)
{
    serialize(model_, value, ParameterSerdesType::PST_STRU);
}

template <typename T>
void ParameterTree<T>::commit_value_changes()
{
    deserialize(model_, &value_, ParameterSerdesType::PST_STRU);
}

template <typename T>
void ParameterTree<T>::commit_value_changes(const T& value)
{
    deserialize(model_, &value, ParameterSerdesType::PST_STRU);
}

template <typename T>
void ParameterTree<T>::serialize(std::shared_ptr<const Parameter> p, void* ptr, ParameterSerdesType type)
{
    std::unordered_map<std::string, std::string> violations;
    {
        std::lock_guard<std::recursive_mutex> gate(serialize_lock_); // 序列化闸门
        std::lock_guard<std::mutex> guard(lock_);
        model_->profile()->serdes_type = type;
        model_->profile()->violations.clear();
        p->serialize(ptr);
        violations.swap(model_->profile()->violations);
    }

    std::unordered_map<std::string, std::pair<std::string, std::string>> exceptions;
    for (auto& violation : violations) {
        const auto index = violation.first;
        auto vp = parameter(parse_index(index)); // 并发 remove 后可能为空, 不能直接解引用
        const auto info = vp ? vp->debug_releation() : std::string();
        exceptions[index] = std::make_pair(violation.second, info);
    }

    if (!exceptions.empty()) {
        throw ViolationException(exceptions);
    }
}

template <typename T>
void ParameterTree<T>::deserialize(std::shared_ptr<Parameter> p, const void* ptr, ParameterSerdesType type)
{
    std::lock_guard<std::recursive_mutex> gate(serialize_lock_); // 序列化闸门
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
T* ParameterTree<T>::mutable_value(std::unique_lock<std::mutex>& holder)
{
    std::unique_lock<std::mutex> guard(lock_);
    holder.swap(guard);
    return &value_;
}

template <typename T>
T* ParameterTree<T>::unsafe_value()
{
    return &value_;
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
    static_assert(is_basic<V>::value || is_enum<V>::value, "set<>() only support basic/enum type");
    std::lock_guard<std::mutex> guard(lock_);
    auto p = std::dynamic_pointer_cast<TraitedParameter<V>>(parameter(parse_index(index)));
    if (!p) {
        throw ParameterNotFound(index);
    }
    return static_cast<V>(p->value);
}

template <typename T>
template <typename V>
void ParameterTree<T>::set(const std::string& index, const V& value)
{
    static_assert(is_basic<V>::value || is_enum<V>::value, "set<>() only support basic/enum type");
    {
        std::lock_guard<std::mutex> guard(lock_);
        auto p = std::dynamic_pointer_cast<TraitedParameter<V>>(parameter(parse_index(index)));
        if (!p) {
            throw ParameterNotFound(index);
        }
        const_cast<V&>(p->value) = value;
    }
    commit_model_changes();
}

template <typename T>
void ParameterTree<T>::set(const std::string& index, const std::string& value)
{
    {
        std::lock_guard<std::mutex> guard(lock_);
        auto p = parameter(parse_index(index));
        if (!p) {
            throw ParameterNotFound(index);
        }
        p->from_string(value);
    }
    commit_model_changes();
}

template <typename T>
void ParameterTree<T>::set(const std::string& index, const char* value)
{
    set(index, std::string(value));
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
    std::lock_guard<std::mutex> guard(lock_);
    model_->profile()->inform_enabled = enable;
}

template <typename T>
void ParameterTree<T>::enable_subtree_inline(bool enable)
{
    // 用序列化闸门而非数据锁: subtree_inline 只在序列化遍历时被后端读取(闸门保护),
    // 与 protobuf build_schema 的临时切换保持一致
    std::lock_guard<std::recursive_mutex> gate(serialize_lock_);
    model_->profile()->subtree_inline = enable;
}

template <typename T>
bool ParameterTree<T>::subtree_inline() const
{
    std::lock_guard<std::recursive_mutex> gate(serialize_lock_);
    return model_->profile()->subtree_inline;
}

template <typename T>
void ParameterTree<T>::reload_subtree(const std::string& index, ParameterSerdesType type)
{
    {
        std::lock_guard<std::recursive_mutex> gate(serialize_lock_); // 序列化闸门
        std::lock_guard<std::mutex> guard(lock_);
        auto p = parameter(parse_index(index));
        if (!p) {
            return;
        }
        auto* sub = dynamic_cast<Subtree*>(p.get());
        if (!sub || sub->path.empty()) {
            return;
        }
        model_->profile()->serdes_type = type; // 子节点反序列化走对应后端(无扩展名时回退主格式)
        sub->load(type);
    }

    commit_model_changes(); // model -> value_, 使后续 save 的 commit_value_changes 不会用旧缓存覆盖
}

template <typename T>
std::vector<std::string> ParameterTree<T>::subtree_indices()
{
    std::lock_guard<std::mutex> guard(lock_);
    std::vector<std::string> indices;
    std::function<void(const std::shared_ptr<Parameter>&)> walk = [&indices, &walk](const std::shared_ptr<Parameter>& p) {
        if (dynamic_cast<Subtree*>(p.get()) != nullptr) {
            indices.push_back(p->index());
        }
        for (const auto& child_pair : p->children()) {
            walk(child_pair.second);
        }
    };
    walk(model_);
    return indices;
}

template <typename T>
bool ParameterTree<T>::visit_subtree(const std::string& index, const std::function<void(Subtree&)>& visitor)
{
    std::lock_guard<std::mutex> guard(lock_);
    auto p = parameter(parse_index(index));
    if (!p) {
        return false;
    }
    auto* sub = dynamic_cast<Subtree*>(p.get());
    if (!sub) {
        return false;
    }
    visitor(*sub);
    return true;
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