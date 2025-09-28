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
            ss << "message " << c.first << " {" << std::endl;
            size_t index = 1;
            for (auto& item : c.second) {
                ss << "    " << item.second.type << " " << item.second.name << " = " << index++ << ";" << std::endl;
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
        parent[parameter->offset].type = protobuf_type<T>();
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
        parent[parameter->offset].type = "int32";
        parent[parameter->offset].name = parameter->subkey;
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
        const auto type_name = detail::simplify_message_name(parameter->readable_detail_type());

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
class ProtobufDefineSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        const auto type_name = is_basic<typename T::value_type>::value ? protobuf_type<typename T::value_type>()
                                                                       : detail::simplify_message_name(parameter->readable_detail_type());

        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& parent = *(ctx.parent_message_content());
        parent[parameter->offset].type = "repeated " + type_name;
        parent[parameter->offset].name = parameter->subkey;

        // SEQUENCE处理自己的下级
        std::string internal_type_name = "__internal_anonymous_message__";
        std::string internal_value_type;
        if (is_stl<typename T::value_type>::value) {
            detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
            parent[parameter->offset].type = "repeated " + internal_type_name;
        }
        auto subparam = ParameterPrototype::create_parameter(parameter->detail, internal_value_type);
        subparam->parent = std::const_pointer_cast<Parameter>(p);
        {
            detail::EnterProtobufDefineMessageGuard guard(ctx, internal_type_name);
            subparam->serialize(out);
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
        const auto type_name = is_basic<typename T::mapped_type>::value ? protobuf_type<typename T::mapped_type>()
                                                                        : detail::simplify_message_name(parameter->readable_detail_type());

        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& msg = *(ctx.parent_message_content());

        const auto key_name = protobuf_type<typename T::key_type>();
        msg[parameter->offset].type = "map<" + (key_name == "bytes" ? "string" : key_name) + ", " + type_name + ">";
        msg[parameter->offset].name = parameter->subkey;

        // MAP处理自己的下级
        std::string internal_type_name = "__internal_anonymous_message__";
        std::string internal_value_type;
        if (is_stl<typename T::mapped_type>::value) {
            detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
            const auto key_name = protobuf_type<typename T::key_type>();
            msg[parameter->offset].type = "map<" + (key_name == "bytes" ? "string" : key_name) + ", " + internal_type_name + ">";
        }

        auto subparam = ParameterPrototype::create_parameter(parameter->detail, internal_value_type);
        subparam->parent = std::const_pointer_cast<Parameter>(p);
        {
            detail::EnterProtobufDefineMessageGuard guard(ctx, internal_type_name);
            subparam->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

// SET
template <typename T>
class ProtobufDefineSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        const auto type_name = is_basic<typename T::value_type>::value ? protobuf_type<typename T::value_type>()
                                                                       : detail::simplify_message_name(parameter->readable_detail_type());

        auto& ctx = *static_cast<detail::ProtobufDefineSerdesContext*>(out);
        auto& msg = *(ctx.parent_message_content());
        msg[parameter->offset].type = "repeated " + type_name;
        msg[parameter->offset].name = parameter->subkey;

        // SET处理自己的下级
        std::string internal_type_name = "__internal_anonymous_message__";
        std::string internal_value_type;
        if (is_stl<typename T::value_type>::value) {
            detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
            msg[parameter->offset].type = "repeated " + internal_type_name;
        }

        auto subparam = ParameterPrototype::create_parameter(parameter->detail, internal_value_type);
        subparam->parent = std::const_pointer_cast<Parameter>(p);
        {
            detail::EnterProtobufDefineMessageGuard guard(ctx, internal_type_name);
            subparam->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        // do nothing
    }
};

}