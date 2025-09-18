#pragma once

#include "serdes/serdes/config.h"

#include "serdes/types/basic.hpp"
#include "serdes/types/enum.hpp"
#include "serdes/types/map.hpp"
#include "serdes/types/object.hpp"
#include "serdes/types/prototype.hpp"
#include "serdes/types/sequence.hpp"
#include "serdes/types/set.hpp"

#include "serdes/types/basic_impl.hpp"
#include "serdes/types/composite_impl.hpp"
#include "serdes/types/enum_impl.hpp"

namespace xvorin::serdes {

class ParameterRegistrar {
public:
    template <typename P, typename M, typename E = void> //[P]arent & [M]ember
    struct RegisterParameter {
    };

    template <typename P, typename M> //[P]arent & [M]ember
    struct RegisterParameter<P, M, typename std::enable_if<is_composite<M>::value>::type> {
        RegisterParameter(std::string subkey, size_t offset, std::string comment = "",
            typename TraitedParameter<M>::inform_type cb = nullptr, std::string verinfo = "")
        {
            constexpr bool enable = std::is_pointer<M>::value || std::is_reference<M>::value || std::is_const<M>::value;
            static_assert(!enable, "Do not support pointer, reference or const type");

            // 保存 父结构体类型 到类型模板容器中
            auto parent = SavePrototypeHelper<P>::save();
            // 保存 成员类型 到类型模板容器中
            SavePrototypeHelper<M>::save();

            // 存在同名成员, 合并成员属性
            auto previous = std::static_pointer_cast<TraitedParameter<M>>(parent->find_child(subkey));
            if (previous) {
                comment = (comment.length() > previous->comment.length()) ? comment : previous->comment;
                verinfo = (verinfo.length() > previous->verinfo.length()) ? verinfo : previous->verinfo;
                cb = (cb) ? cb : previous->inform;
            }
            // 挂接成员类型: 记录偏移量 用于针对结构体的序列化与反序列化; 记录回调函数 用于变更通知
            auto child = std::make_shared<TraitedParameter<M>>(std::move(subkey), offset, std::move(comment), std::move(cb), std::move(verinfo));
            parent->insert_child(child);
        }
    };

    template <typename P, typename M> //[P]arent & [M]ember
    struct RegisterParameter<P, M, typename std::enable_if<!is_composite<M>::value>::type> {
        RegisterParameter(std::string subkey, size_t offset, std::string comment = "",
            typename TraitedParameter<M>::inform_type cb = nullptr, std::string verinfo = "")
        {
            constexpr bool enable = std::is_pointer<M>::value || std::is_reference<M>::value || std::is_const<M>::value;
            static_assert(!enable, "Do not support pointer, reference or const type");

            // 保存 父结构体类型 到类型模板容器中
            auto parent = SavePrototypeHelper<P>::save();
            // 保存 成员类型 到类型模板容器中
            SavePrototypeHelper<M>::save();

            // 存在同名成员, 合并成员属性
            auto previous = std::static_pointer_cast<TraitedParameter<M>>(parent->find_child(subkey));
            if (previous) {
                comment = (comment.length() > previous->comment.length()) ? comment : previous->comment;
                verinfo = (verinfo.length() > previous->verinfo.length()) ? verinfo : previous->verinfo;
                cb = (cb) ? cb : previous->inform;
            }
            // 挂接成员类型: 记录偏移量 用于针对结构体的序列化与反序列化; 记录回调函数 用于变更通知
            auto child = std::make_shared<TraitedParameter<M>>(std::move(subkey), offset, std::move(comment), std::move(cb), std::move(verinfo));
            // 设置基础类型成员默认值
            P value;
            child->value = *reinterpret_cast<M*>(reinterpret_cast<char*>(&value) + offset);
            parent->insert_child(child);
        }
    };

    template <typename P> //[P]arent
    struct HiddenParameter {
        explicit HiddenParameter(const std::string& subkey)
        {
            // 保存 父结构体类型 到类型模板容器中
            auto parent = SavePrototypeHelper<P>::save();
            // 删除 成员
            auto iter = parent->children.find(subkey);
            if (iter != parent->children.end()) {
                parent->children.erase(iter);
            }
        }
    };

    template <typename R> //[R]oot
    struct RegisterRoot {
        RegisterRoot(size_t offset, const std::string& comment = "",
            typename TraitedParameter<R>::inform_type cb = nullptr, const std::string& verinfo = "")
        {
            constexpr bool enable = std::is_pointer<R>::value || std::is_reference<R>::value || std::is_const<R>::value;
            static_assert(!enable, "Do not support pointer, reference or const type");

            // 保存 结构体类型 到类型模板容器中
            auto root = SavePrototypeHelper<R>::save();

            // 挂接成员类型: 记录回调函数 用于变更通知
            if (!comment.empty()) {
                const_cast<std::string&>(root->comment) = comment;
            }

            if (!verinfo.empty()) {
                const_cast<std::string&>(root->verinfo) = verinfo;
            }
            if (cb) {
                const_cast<typename TraitedParameter<R>::inform_type&>(root->inform) = std::move(cb);
            }
        }
    };

    template <typename B, typename D> //[B]ase & [D]rive
    struct RegisterInheritRelation {
        RegisterInheritRelation()
        {
            constexpr bool base_enable = std::is_pointer<B>::value || std::is_reference<B>::value || std::is_const<B>::value;
            constexpr bool drive_enable = std::is_pointer<D>::value || std::is_reference<D>::value || std::is_const<D>::value;
            static_assert(!base_enable && !drive_enable, "Do not support pointer, reference or const type!");
            static_assert(is_object<B>::value && is_object<D>::value, "Must be struct or class!");
            static_assert(std::is_base_of<B, D>::value, "Has no inheritance relationship!");

            // 保存继承关系
            SavePrototypeHelper<B>::save();
            auto drive = SavePrototypeHelper<D>::save();
            drive->bases.insert(typeid(B));
        }
    };

private:
    template <typename T, typename E = void>
    struct SavePrototypeHelper {
    };

    template <typename T>
    struct SavePrototypeHelper<T, typename std::enable_if<!is_stl<T>::value>::type> {
        using W = TraitedParameter<T>; // [W]rapper
        static std::shared_ptr<W> save()
        {
            auto t = ParameterPrototype::prototype<W>(typeid(T));
            return t ? t : (t = std::make_shared<W>("prototype", 0, "", nullptr, ""), ParameterPrototype::insert_prototype(typeid(T), t), t);
        }
    };

    template <typename T>
    struct SavePrototypeHelper<T, typename std::enable_if<is_stl<T>::value>::type> {
        using W = TraitedParameter<T>; // [W]rapper
        static std::shared_ptr<W> save()
        {
            SavePrototypeHelper<typename W::subtype>::save(); // 递归保存子类型
            auto t = ParameterPrototype::prototype<W>(typeid(T));
            return t ? t : (t = std::make_shared<W>("prototype", 0, "", nullptr, ""), ParameterPrototype::insert_prototype(typeid(T), t), t);
        }
    };
};

}

#define PARAMETER_HELPER_NAME(n) PARAMETER_HELPER_NAME_LEVEL1(n)
#define PARAMETER_HELPER_NAME_LEVEL1(n) PARAMETER_HELPER_NAME_LEVEL2(n)
#define PARAMETER_HELPER_NAME_LEVEL2(n) ParameterRegister##n

#define DEFINE_PARAM(P, m, ...)                                                     \
    static xvorin::serdes::ParameterRegistrar::RegisterParameter<P, decltype(P::m)> \
        PARAMETER_HELPER_NAME(__COUNTER__) { #m, (size_t)(&((P*)0)->m), ##__VA_ARGS__ };

#define HIDDEN_PARAM(P, m)                                        \
    static xvorin::serdes::ParameterRegistrar::HiddenParameter<P> \
        PARAMETER_HELPER_NAME(__COUNTER__) { #m };

#define DEFINE_ROOT(R, ...)                                    \
    static xvorin::serdes::ParameterRegistrar::RegisterRoot<R> \
        PARAMETER_HELPER_NAME(__COUNTER__) { 0, ##__VA_ARGS__ };

#define DEFINE_INHERIT(B, D)                                                 \
    static xvorin::serdes::ParameterRegistrar::RegisterInheritRelation<B, D> \
        PARAMETER_HELPER_NAME(__COUNTER__);
