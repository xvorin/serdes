#pragma once

#include "serdes/serdes/protobuf/protobuf_type.hpp"
#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

#include "serdes/types/map_key.hpp"

#include <iostream>
#include <type_traits>

namespace xvorin::serdes {

namespace detail {

class InMemorySourceTree : public google::protobuf::compiler::SourceTree {
public:
    // 添加文件到内存映射
    void AddFile(const std::string& filename, const std::string& content)
    {
        files_[filename] = content;
    }

    // 实现Open方法，返回内存中的数据流
    virtual google::protobuf::io::ZeroCopyInputStream* Open(const std::string& filename) override
    {
        auto it = files_.find(filename);
        if (it == files_.end()) {
            last_error_ = "File not found: " + filename;
            return nullptr;
        }
        // 使用ArrayInputStream包装字符串内容
        return new google::protobuf::io::ArrayInputStream(it->second.data(), it->second.size());
    }

    // 错误处理
    virtual std::string GetLastErrorMessage() override
    {
        return last_error_;
    }

private:
    std::map<std::string, std::string> files_;
    std::string last_error_;
};

class ProtobufMessage {
public:
    ProtobufMessage(google::protobuf::Message* msg, const google::protobuf::Descriptor* desc,
        int index = -1, std::string value_name = "")
        : msg_(msg)
        , desc_(desc)
        , repeat_index_(index)
        , value_name_(std::move(value_name))
    {
        assert(msg_ && desc_);
    }

    std::string type_name() const
    {
        return desc_->name();
    }

    google::protobuf::Message* message()
    {
        return msg_;
    }

    const google::protobuf::Descriptor* descriptor() const
    {
        return desc_;
    }

    int repeat_index() const
    {
        return repeat_index_;
    }

    void set_repeat_index(int index)
    {
        repeat_index_ = index;
    }

    std::string value_name() const
    {
        return value_name_;
    }

    void set_value_name(std::string name)
    {
        value_name_ = std::move(name);
    }

private:
    google::protobuf::Message* msg_ = nullptr; // 可以获取reflection
    const google::protobuf::Descriptor* desc_ = nullptr; // 可以获取 field

    /// 以下为非必须参数, 用于支持protobuf的map类型和repeated类型(映射为STL相关数据结构)
    int repeat_index_ = -1; //-1 means not repeat, 用于标记repeated类型, 可以影响子节点的赋值行为
    std::string value_name_; // value_name_ : type_name_在参数树特定位置处的变量名, 用于子节点进行FindFieldByName操作
};

class ProtobufSerdesContext {
public:
    ProtobufSerdesContext(const std::string& pbdef)
    {
        load_proto_schema(pbdef, &descriptor_pool_); // 创建动态池和工厂
    }

    const google::protobuf::Descriptor* get_descriptor(const std::string& type_name)
    {
        return descriptor_pool_.FindMessageTypeByName(type_name);
    }

    google::protobuf::Message* create_message(const std::string& type_name)
    {
        auto desc = get_descriptor(type_name);
        return dynamic_factory_.GetPrototype(desc)->New();
    }

    ProtobufMessage mutable_message(ProtobufMessage* parent, const std::string& value_name)
    {
        auto parent_desc = parent->descriptor();
        auto parent_msg = parent->message();
        auto parent_refl = parent_msg->GetReflection();

        auto child_field = parent_desc->FindFieldByName(value_name);

        auto child_msg = parent_refl->MutableMessage(parent_msg, child_field);
        auto child_desc = child_field->message_type();

        return ProtobufMessage(child_msg, child_desc);
    }

    ProtobufMessage append_message(ProtobufMessage* parent, const std::string& value_name)
    {
        auto parent_desc = parent->descriptor();
        auto parent_msg = parent->message();
        auto parent_refl = parent_msg->GetReflection();

        auto child_field = parent_desc->FindFieldByName(value_name);

        auto child_msg = parent_refl->AddMessage(parent_msg, child_field);
        auto child_desc = child_field->message_type();

        return ProtobufMessage(child_msg, child_desc);
    }

    size_t message_field_size(ProtobufMessage* parent, const std::string& value_name)
    {
        auto field = parent->descriptor()->FindFieldByName(value_name);
        return parent->message()->GetReflection()->FieldSize(*(parent->message()), field);
    }

    ProtobufMessage mutable_repeated_message(ProtobufMessage* parent, const std::string& value_name, int index)
    {
        auto field = parent->descriptor()->FindFieldByName(value_name);
        assert(field->is_repeated());
        auto& msg = parent->message()->GetReflection()->GetRepeatedMessage(*(parent->message()), field, index);
        return ProtobufMessage(const_cast<google::protobuf::Message*>(&msg), field->message_type());
    }

    std::string to_binary_string()
    {
        return root_ ? root_->SerializeAsString() : "";
    }

    std::string to_txt_string()
    {
        std::string text;
        google::protobuf::TextFormat::PrintToString(*root_, &text);
        return text;
    }

    std::string to_debug_string(bool simplified)
    {
        return root_ ? (simplified ? root_->ShortDebugString() : root_->DebugString()) : "";
    }

    bool from_binary_string(const std::string& type_name, const std::string& pbbin)
    {
        set_root(create_message(type_name));
        return root_ && root_->ParseFromString(pbbin);
    }

    bool from_txt_string(const std::string& type_name, const std::string& pbtxt)
    {
        set_root(create_message(type_name));
        return root_ && google::protobuf::TextFormat::ParseFromString(pbtxt, root_);
    }

    ProtobufMessage* get_current_msg()
    {
        return (msgs.empty()) ? nullptr : &(msgs.top());
    }

    google::protobuf::Message* root()
    {
        return root_;
    }

    void set_root(google::protobuf::Message* root)
    {
        assert(!root_);
        root_ = root;
    }

private:
    // 创建类型系统
    bool load_proto_schema(const std::string& pbdef, google::protobuf::DescriptorPool* pool)
    {
        InMemorySourceTree source_tree;
        source_tree.AddFile("dynamic.proto", pbdef); // 添加proto内容到内存，文件名需与import语句匹配

        // 创建Importer，使用自定义的SourceTree
        google::protobuf::compiler::Importer importer(&source_tree, nullptr);
        auto file = importer.Import("dynamic.proto");
        if (!file) {
            return false;
        }

        // 将描述符复制到目标池
        google::protobuf::FileDescriptorProto fproto;
        file->CopyTo(&fproto);
        return pool->BuildFile(fproto);
    }

public:
    std::stack<ProtobufMessage> msgs;

private:
    google::protobuf::DescriptorPool descriptor_pool_;
    google::protobuf::DynamicMessageFactory dynamic_factory_ { &descriptor_pool_ };
    google::protobuf::Message* root_ = nullptr;
};

struct EnterProtobufMessageGuard {
    EnterProtobufMessageGuard(ProtobufSerdesContext& ctx, ProtobufMessage& msg)
        : ctx_(ctx)
    {
        ctx_.msgs.push(msg);
    }
    ~EnterProtobufMessageGuard()
    {
        ctx_.msgs.pop();
    }

    ProtobufSerdesContext& ctx_;
};

}

template <typename T, typename E = void> //[T]ype & [E]nable
class ProtobufSerdes : public Serdes {
};

// BASIC
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_basic<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.get_current_msg();
        assert(parent);

        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();

        if (parent->repeat_index() >= 0) { // repeated 修饰的基本类型
            auto field = parent->descriptor()->FindFieldByName(value_name);
            detail::protobuf_add_repeated_value(parent->message(), field, parameter->value);
        } else {
            auto field = parent->descriptor()->FindFieldByName(value_name);
            detail::protobuf_set_value(parent->message(), field, parameter->value);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        auto field = parent->descriptor()->FindFieldByName(value_name);

        if (parent->repeat_index() >= 0) { // repeated 修饰的基本类型
            parameter->value = detail::protobuf_get_repeated_value<T>(parent->message(), field, parent->repeat_index());
        } else {
            if (field) {
                parameter->value = detail::protobuf_get_value<T>(parent->message(), field);
            }
        }
    }
};

// ENUM
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_enum<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.get_current_msg();

        if (parent->repeat_index() >= 0) { // repeated 修饰的enum类型
            assert(!parent->value_name().empty()); // 一定会在上一级虚拟一个带value_name的消息层
            auto field = parent->descriptor()->FindFieldByName(parent->value_name());
            detail::protobuf_add_repeated_value(parent->message(), field, static_cast<int>(parameter->value));
        } else {
            auto field = parent->descriptor()->FindFieldByName(parameter->subkey);
            detail::protobuf_set_value(parent->message(), field, static_cast<int>(parameter->value));
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        auto field = parent->descriptor()->FindFieldByName(value_name);

        if (parent->repeat_index() >= 0) { // repeated 修饰的基本类型
            parameter->value = static_cast<T>(detail::protobuf_get_repeated_value<int>(parent->message(), field, parent->repeat_index()));
        } else {
            if (field) {
                parameter->value = static_cast<T>(detail::protobuf_get_value<int>(parent->message(), field));
            }
        }
    }
};

// OBJECT
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));

        const auto type_name = detail::simplify_message_name(parameter->readable_detail_type());
        auto parent = ctx.get_current_msg();

        if (!parent) { // 没有上一级 说明是根
            ctx.set_root(ctx.create_message(type_name));
            auto current = detail::ProtobufMessage(ctx.root(), ctx.get_descriptor(type_name));
            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            for (auto& child : parameter->sorted_children()) {
                child->serialize(out);
            }
            return;
        }

        // 有上级, 需要基于上级创建本级消息 append_message:适用于repeated 消息; mutable_message:适用于普通消息
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        auto current = (parent->repeat_index() >= 0) ? ctx.append_message(parent, value_name)
                                                     : ctx.mutable_message(parent, value_name);
        detail::EnterProtobufMessageGuard guard(ctx, current);
        for (auto& child : parameter->sorted_children()) {
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.get_current_msg();
        const auto type_name = detail::simplify_message_name(parameter->readable_detail_type());

        if (!parent) { // 没有上一级 说明是根
            auto current = detail::ProtobufMessage(ctx.root(), ctx.get_descriptor(type_name));
            detail::EnterProtobufMessageGuard guard(ctx, current);
            for (auto& child : parameter->sorted_children()) {
                child->deserialize(in);
            }
            return;
        }

        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        auto current = (parent->repeat_index() >= 0) ? ctx.mutable_repeated_message(parent, value_name, parent->repeat_index())
                                                     : ctx.mutable_message(parent, value_name);

        detail::EnterProtobufMessageGuard guard(ctx, current);
        for (auto& child : parameter->sorted_children()) {
            child->deserialize(in);
        }
    }
};

// SEQUENCE
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_sequence<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();

        int repeat_index = 0;
        for (auto& child : parameter->sorted_children()) {
            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            auto current = *parent;
            current.set_value_name(value_name); // 传递value_name,用于下一级寻找field

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                std::string internal_type_name;
                std::string internal_value_type;
                detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
                current = ctx.append_message(parent, value_name);
                current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
            }

            // 添加repeat标志
            current.set_repeat_index(repeat_index++);

            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));

        const auto type_name = detail::simplify_message_name(parameter->readable_detail_type());
        auto parent = ctx.get_current_msg();

        parameter->children.clear();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        for (size_t i = 0; i < ctx.message_field_size(parent, value_name); ++i) {
            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            auto current = *parent;
            current.set_value_name(value_name); // 传递value_name,用于下一级寻找field

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                std::string internal_type_name;
                std::string internal_value_type;
                detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
                current = ctx.mutable_repeated_message(parent, value_name, i);
                current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
            }

            // 添加repeat标志
            current.set_repeat_index(i);

            // 处理下一级
            const auto subkey = std::to_string(i);
            detail::EnterProtobufMessageGuard guard(ctx, current);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(in);
        }
    }
};

// MAP
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_map<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();

        auto field = parent->descriptor()->FindFieldByName(value_name);
        assert(field && field->is_map());

        auto descriptor = field->message_type();
        auto key_field = descriptor->FindFieldByName("key");
        auto value_field = descriptor->FindFieldByName("value");

        for (auto& child : parameter->sorted_children()) {
            // 添加一个 map 条目
            auto entry = parent->message()->GetReflection()->AddMessage(parent->message(), field);

            // set key
            {
                auto subkey = from_string<typename T::key_type>(child->subkey);
                detail::protobuf_set_value(entry, key_field, subkey);
            }

            // set value
            {
                std::string internal_type_name;
                std::string internal_value_type;
                detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);

                // 创建虚拟的消息层(下一级非STL)
                auto current = detail::ProtobufMessage(entry, descriptor);
                current.set_value_name("value");

                // 创建真实的消息层(下一级是STL 需要当前层级插入一个真实的消息层)
                if (is_stl<typename T::mapped_type>::value) {
                    auto child_proto_msg = entry->GetReflection()->MutableMessage(entry, value_field);
                    current = detail::ProtobufMessage(child_proto_msg, ctx.get_descriptor(internal_type_name));
                    current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
                }

                // 处理下一级
                detail::EnterProtobufMessageGuard guard(ctx, current);
                child->serialize(out);
            }
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();

        auto field = parent->descriptor()->FindFieldByName(value_name);
        assert(field && field->is_map());

        auto descriptor = field->message_type();
        auto key_field = descriptor->FindFieldByName("key");
        auto value_field = descriptor->FindFieldByName("value");

        parameter->children.clear();
        for (size_t i = 0; i < ctx.message_field_size(parent, value_name); ++i) {
            const auto& entry = parent->message()->GetReflection()->GetRepeatedMessage(*(parent->message()), field, i);

            // get key
            const auto subkey = to_string<typename T::key_type>(detail::protobuf_get_value<typename T::key_type>(&entry, key_field));

            // get value
            std::string internal_type_name;
            std::string internal_value_type;
            detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);

            // 创建虚拟的消息层(下一级非STL)
            auto current = detail::ProtobufMessage(const_cast<google::protobuf::Message*>(&entry), descriptor);
            current.set_value_name("value");

            // 创建真实的消息层(下一级是STL 需要当前层级插入一个真实的消息层)
            if (is_stl<typename T::mapped_type>::value) {
                const auto& child_proto_msg = entry.GetReflection()->GetMessage(entry, value_field);
                current = detail::ProtobufMessage(const_cast<google::protobuf::Message*>(&child_proto_msg), ctx.get_descriptor(internal_type_name));
                current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
            }

            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(in);
        }
    }
};

// SET
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.get_current_msg();
        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();

        int repeat_index = 0;
        for (auto& child : parameter->sorted_children()) {
            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            auto current = *parent;
            current.set_value_name(value_name); // 传递value_name,用于下一级寻找field

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                std::string internal_type_name;
                std::string internal_value_type;
                detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
                current = ctx.append_message(parent, value_name);
                current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
            }

            // 添加repeat标志
            current.set_repeat_index(repeat_index++);

            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));

        const auto type_name = detail::simplify_message_name(parameter->readable_detail_type());
        auto parent = ctx.get_current_msg();

        parameter->children.clear();

        const auto value_name = parent->value_name().empty() ? parameter->subkey : parent->value_name();
        auto field = parent->descriptor()->FindFieldByName(value_name);
        assert(field->is_repeated());

        const size_t size = parent->message()->GetReflection()->FieldSize(*(parent->message()), field);
        for (size_t i = 0; i < size; ++i) {
            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            auto current = *parent;
            current.set_value_name(value_name); // 传递value_name,用于下一级寻找field

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                std::string internal_type_name;
                std::string internal_value_type;
                detail::make_internal_name(parameter->readable_detail_type(), internal_type_name, internal_value_type);
                current = ctx.mutable_repeated_message(parent, value_name, i);
                current.set_value_name(internal_value_type); // 传递value_name,用于下一级寻找field
            }

            // 添加repeat标志
            current.set_repeat_index(i);

            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            const auto subkey = std::to_string(i);
            parameter->children[subkey] = ParameterPrototype::create_parameter(parameter->detail, subkey);
            parameter->children[subkey]->parent = parameter;
            parameter->children[subkey]->deserialize(in);
        }
    }
};

}