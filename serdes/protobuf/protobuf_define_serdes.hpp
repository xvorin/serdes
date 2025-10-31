#pragma once

#include "serdes/serdes/protobuf/protobuf_type.hpp"
#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include <algorithm>
#include <map>
#include <stack>

#include <iostream>

namespace xvorin::serdes {

namespace detail {

struct ProtobufDefineSerdesItem {
    std::string type;
    std::string name;
};

struct ProtobufDefineSerdesContext {
    std::stack<std::string> messages;
    std::unordered_map<std::string, std::map<int, ProtobufDefineSerdesItem>> contents;

    std::string to_string(const std::string& pkg = "")
    {
        std::stringstream ss;
        ss << "syntax = " << R"("proto3";)" << std::endl;

        if (!pkg.empty()) {
            ss << "package " << pkg << ";" << std::endl;
        }

        for (auto& c : contents) {
            if ("__internal_anonymous_message__" == c.first) {
                continue;
            }

            ss << std::endl;
            if (!c.second.empty() && c.second.begin()->first < 0) { // trick! first < 0 for enum
                ss << "enum " << c.first << " {" << std::endl;
                size_t index = 0;
                for (auto& item : c.second) {
                    ss << "    " << item.second.type << " = " << index++ << ";" << std::endl;
                }
            } else {
                ss << "message " << c.first << " {" << std::endl;
                size_t index = 1;
                for (auto& item : c.second) {
                    ss << "    " << item.second.type << " " << item.second.name << " = " << index++ << ";" << std::endl;
                }
            }
            ss << "}" << std::endl;
        }
        return ss.str();
    }

    bool contains(const std::string& name)
    {
        return contents.find(name) != contents.end();
    }

    const std::string& parent_message()
    {
        static std::string empty_message;
        return messages.empty() ? empty_message : messages.top();
    }

    std::map<int, ProtobufDefineSerdesItem>* parent_message_content()
    {
        if (messages.empty()) {
            return nullptr;
        }
        assert(contains(messages.top()));
        return &contents[messages.top()];
    }
};

struct EnterProtobufDefineMessageGuard {
    EnterProtobufDefineMessageGuard(ProtobufDefineSerdesContext& ctx, const std::string& message)
        : ctx_(ctx)
    {
        ctx_.contents[message] = std::map<int, ProtobufDefineSerdesItem>();
        ctx_.messages.push(message);
    }
    ~EnterProtobufDefineMessageGuard()
    {
        ctx_.messages.pop();
    }

    ProtobufDefineSerdesContext& ctx_;
};

}

template <typename T, typename E = void> //[T]ype & [E]nable
class ProtobufDefineSerdes : public Serdes {
};

// BASIC
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());

        parent[parameter->offset].type = detail::Pbtraits<T>::type();
        parent[parameter->offset].name = parameter->subkey;
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// ENUM
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());

        const auto type_name = detail::Pbtraits<T>::type();
        parent[parameter->offset].type = type_name;
        parent[parameter->offset].name = parameter->subkey;

        if (!ctx.contains(type_name)) {
            detail::EnterProtobufDefineMessageGuard guard(ctx, type_name);
            auto& parent = *(ctx.parent_message_content());
            int index = 0;
            for (auto iter = parameter->enum_mapping.rbegin(); iter != parameter->enum_mapping.rend(); iter++) {
                parent[index--].type = iter->second;
            }
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// OBJECT
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        const auto type_name = detail::Pbtraits<T>::type();

        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto parent = ctx.parent_message_content();
        const auto message = ctx.parent_message();

        if (parent != nullptr && message != "__internal_anonymous_message__") {
            // object作为下级,把自己挂接上去
            (*parent)[parameter->offset].type = type_name;
            (*parent)[parameter->offset].name = parameter->subkey;
        }

        // object处理自己的下级
        if (!ctx.contains(type_name)) {
            detail::EnterProtobufDefineMessageGuard guard(ctx, type_name);
            for (auto& child : parameter->sorted_children()) {
                child->serialize(out);
            }
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// SEQUENCE
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_sequence<T>::value || is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());

        const std::string child_type_name = detail::Pbtraits<typename T::value_type>::type();
        const std::string child_value_name = detail::Pbtraits<typename T::value_type>::value();

        parent[parameter->offset].type = "repeated " + child_type_name;
        parent[parameter->offset].name = parameter->subkey;

        if (is_basic<typename T::value_type>::value) { // 子类型为基本类型
            return;
        }

        // 以下都是复杂类型, 需要插入中间消息, 保证它们的子节点: 1.不再继续存在于当前message内, 2.但同时需要另行建立一个message
        //    对于子类型为 结构体的 另行建立的message为结构体名称,字段不需要在下层中做向上挂接的动作，但是需要通过触发下层记录结构体自身的完整消息内容
        //    对于子类型为 STL的   另行建立的message需要内部生成一个internal_type_name(包装消息), 因为protobuf 不支持连续的repeated定义, 类似: repeated repeated ... int
        auto child = ParameterPrototype::create_parameter(parameter->detail, child_value_name, std::const_pointer_cast<Parameter>(p));
        {
            const bool anonymous = (is_object<typename T::value_type>::value || is_enum<typename T::value_type>::value);
            const auto type_name = anonymous ? "__internal_anonymous_message__" : child_type_name;
            detail::EnterProtobufDefineMessageGuard guard(ctx, type_name);
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// MAP
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());

        const bool str_as_key = (detail::Pbtraits<typename T::key_type>::type() == "bytes" || is_enum<typename T::key_type>::value);
        const std::string mapkey_type_name = str_as_key ? "string" : detail::Pbtraits<typename T::key_type>::type();
        const std::string child_type_name = detail::Pbtraits<typename T::mapped_type>::type();
        const std::string child_value_name = detail::Pbtraits<typename T::mapped_type>::value();

        parent[parameter->offset].type = "map<" + mapkey_type_name + ", " + child_type_name + ">";
        parent[parameter->offset].name = parameter->subkey;

        if (is_basic<typename T::mapped_type>::value) { // 子类型为基本类型
            return;
        }

        // 以下都是复杂类型, 需要插入中间消息, 保证它们的子节点: 1.不再继续存在于当前message内, 2.但同时需要另行建立一个message
        //    对于子类型为 结构体的 另行建立的message为结构体名称,字段不需要在下层中做向上挂接的动作，但是需要通过触发下层记录结构体自身的完整消息内容
        //    对于子类型为 STL的   另行建立的message需要内部生成一个internal_type_name(包装消息), 因为protobuf 不支持连续的repeated定义, 类似: repeated repeated ... int
        auto child = ParameterPrototype::create_parameter(parameter->detail, child_value_name, std::const_pointer_cast<Parameter>(p));
        {
            const bool anonymous = (is_object<typename T::mapped_type>::value || is_enum<typename T::mapped_type>::value);
            const auto type_name = anonymous ? "__internal_anonymous_message__" : child_type_name;
            detail::EnterProtobufDefineMessageGuard guard(ctx, type_name);
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// PTR
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());

        const std::string child_type_name = detail::Pbtraits<typename T::element_type>::type();
        const std::string child_value_name = detail::Pbtraits<typename T::element_type>::value();

        parent[parameter->offset].type = child_type_name;
        parent[parameter->offset].name = parameter->subkey;

        if (is_basic<typename T::element_type>::value) { // 子类型为基本类型
            return;
        }

        // 以下都是复杂类型, 需要插入中间消息, 保证它们的子节点: 1.不再继续存在于当前message内, 2.但同时需要另行建立一个message
        //    对于子类型为 结构体的 另行建立的message为结构体名称,字段不需要在下层中做向上挂接的动作，但是需要通过触发下层记录结构体自身的完整消息内容
        //    对于子类型为 STL的   另行建立的message需要内部生成一个internal_type_name(包装消息), 因为protobuf 不支持连续的repeated定义, 类似: repeated repeated ... int
        auto child = ParameterPrototype::create_parameter(parameter->detail, child_value_name, std::const_pointer_cast<Parameter>(p));
        {
            const bool anonymous = (is_object<typename T::element_type>::value || is_enum<typename T::element_type>::value);
            const auto type_name = anonymous ? "__internal_anonymous_message__" : child_type_name;
            detail::EnterProtobufDefineMessageGuard guard(ctx, type_name);
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};
}