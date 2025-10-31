#pragma once

#include "serdes/serdes/protobuf/protobuf_type.hpp"
#include "serdes/serdes/serdes.hpp"
#include "serdes/types/traits.hpp"

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
        auto iter = files_.find(filename);
        if (iter == files_.end()) {
            last_error_ = "File not found: " + filename;
            return nullptr;
        }
        // 使用ArrayInputStream包装字符串内容
        return new google::protobuf::io::ArrayInputStream(iter->second.data(), iter->second.size());
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
    ProtobufMessage() = default;

    ProtobufMessage(google::protobuf::Message* msg)
        : msg_(msg)
    {
        assert(msg_);
    }

    std::string type_name() const
    {
        return msg_->GetDescriptor()->name();
    }

    google::protobuf::Message* message()
    {
        return msg_;
    }

    const google::protobuf::Descriptor* descriptor() const
    {
        return msg_->GetDescriptor();
    }

    int repeat_index() const
    {
        return repeat_index_;
    }

    void set_repeat_index(int index)
    {
        repeat_index_ = index;
    }

    bool is_repeated() const
    {
        return repeat_index_ >= 0;
    }

    std::string field_name() const
    {
        return field_name_;
    }

    void set_field_name(std::string name)
    {
        field_name_ = std::move(name);
    }

private:
    google::protobuf::Message* msg_ = nullptr; // 可以获取reflection

    /// 以下为非必须参数, 用于支持protobuf的map类型和repeated类型(映射为STL相关数据结构)
    int repeat_index_ = -1; //-1 means not repeat, 用于标记repeated类型, 可以影响子节点的赋值行为
    std::string field_name_; // value_name_ : type_name_在参数树特定位置处的变量名, 用于子节点进行FindFieldByName操作
};

class ProtobufMessageFactory {
public:
    ProtobufMessageFactory(std::string package, const std::string& pbdef)
    {
        package_.swap((package.empty() ? package : package.append(".")));
        load_proto_schema(pbdef, &descriptor_pool_); // 创建动态池和工厂
    }

    const google::protobuf::Descriptor* get_descriptor(const std::string& type_name)
    {
        return descriptor_pool_.FindMessageTypeByName(package_ + type_name);
    }

    google::protobuf::Message* create_message(const std::string& type_name)
    {
        auto desc = get_descriptor(type_name);
        return dynamic_factory_.GetPrototype(desc)->New();
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

private:
    std::string package_;
    google::protobuf::DescriptorPool descriptor_pool_;
    google::protobuf::DynamicMessageFactory dynamic_factory_ { &descriptor_pool_ };
};

class ProtobufSerdesContext {
public:
    ProtobufSerdesContext(std::shared_ptr<ProtobufMessageFactory> factory)
        : factory_(factory)
    {
    }

    const google::protobuf::Descriptor* get_descriptor(const std::string& type_name)
    {
        return factory_->get_descriptor(type_name);
    }

    google::protobuf::Message* create_message(const std::string& type_name)
    {
        return factory_->create_message(type_name);
    }

    ProtobufMessage mutable_message(ProtobufMessage* parent, const std::string& field_name)
    {
        auto parent_desc = parent->descriptor();
        auto parent_mesg = parent->message();

        auto child_field = parent_desc->FindFieldByName(field_name);
        auto child_mesg = parent_mesg->GetReflection()->MutableMessage(parent_mesg, child_field);

        return ProtobufMessage(child_mesg);
    }

    ProtobufMessage append_message(ProtobufMessage* parent, const std::string& field_name)
    {
        auto parent_desc = parent->descriptor();
        auto parent_mesg = parent->message();

        auto child_field = parent_desc->FindFieldByName(field_name);
        auto child_mesg = parent_mesg->GetReflection()->AddMessage(parent_mesg, child_field);

        return ProtobufMessage(child_mesg);
    }

    size_t message_field_size(ProtobufMessage* parent, const std::string& field_name)
    {
        auto parent_desc = parent->descriptor();
        auto parent_mesg = parent->message();

        auto child_field = parent_desc->FindFieldByName(field_name);

        return parent_mesg->GetReflection()->FieldSize(*parent_mesg, child_field);
    }

    ProtobufMessage mutable_repeated_message(ProtobufMessage* parent, const std::string& field_name, int index)
    {
        auto parent_desc = parent->descriptor();
        auto parent_mesg = parent->message();

        auto child_field = parent_desc->FindFieldByName(field_name);
        auto child_mesg = parent_mesg->GetReflection()->MutableRepeatedMessage(parent_mesg, child_field, index);
        assert(child_field->is_repeated());

        return ProtobufMessage(child_mesg);
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
        return root_ && google::protobuf::TextFormat::ParseFromString(pbtxt, root());
    }

    ProtobufMessage* parent_message()
    {
        return (msgs.empty()) ? nullptr : &(msgs.top());
    }

    google::protobuf::Message* root()
    {
        return root_.get();
    }

    void set_root(google::protobuf::Message* root)
    {
        root_.reset(root); // unique_ptr接管所有权
    }

private:
    std::shared_ptr<ProtobufMessageFactory> factory_;
    std::unique_ptr<google::protobuf::Message> root_;
    std::stack<ProtobufMessage> msgs;

    friend struct EnterProtobufMessageGuard;
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
class ProtobufSerdes<T, typename std::enable_if<is_basic<T>::value && !std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field) {
            return;
        }

        if (parent->is_repeated()) { // repeated 修饰的基本类型
            detail::protobuf_add_repeated_value(parent->message(), field, parameter->value);
        } else {
            detail::protobuf_set_value(parent->message(), field, parameter->value);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field) {
            return;
        }

        if (parent->is_repeated()) { // repeated 修饰的基本类型
            parameter->value = detail::protobuf_get_repeated_value<T>(parent->message(), field, parent->repeat_index());
        } else {
            parameter->value = detail::protobuf_get_value<T>(parent->message(), field);
        }
    }
};

// BASIC-buffer
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<std::is_same<T, buffer>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field) {
            return;
        }

        if (parent->is_repeated()) { // repeated 修饰的基本类型
            detail::protobuf_add_repeated_value(parent->message(), field, static_cast<std::string>(parameter->value));
        } else {
            detail::protobuf_set_value(parent->message(), field, static_cast<std::string>(parameter->value));
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field) {
            return;
        }

        if (parent->is_repeated()) { // repeated 修饰的基本类型
            parameter->value = detail::protobuf_get_repeated_value<std::string>(parent->message(), field, parent->repeat_index());
        } else {
            parameter->value = detail::protobuf_get_value<std::string>(parent->message(), field);
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
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field || field->type() != google::protobuf::FieldDescriptor::TYPE_ENUM) {
            return;
        }

        auto value = field->enum_type()->FindValueByName(Converter<T>::to_string(parameter->value));

        if (parent->is_repeated()) { // repeated 修饰的enum类型
            detail::protobuf_add_repeated_value(parent->message(), field, value);
        } else {
            detail::protobuf_set_value(parent->message(), field, value);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        if (!field || field->type() != google::protobuf::FieldDescriptor::TYPE_ENUM) {
            return;
        }

        const google::protobuf::EnumValueDescriptor* value = nullptr;
        if (parent->is_repeated()) { // repeated 修饰的基本类型
            value = detail::protobuf_get_repeated_value<decltype(value)>(parent->message(), field, parent->repeat_index());
        } else {
            value = detail::protobuf_get_value<decltype(value)>(parent->message(), field);
        }
        parameter->value = Converter<T>::from_string(value->name());
    }
};

// OBJECT
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_object<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.parent_message();

        if (!parent) { // 没有上一级 说明是根
            ctx.set_root(ctx.create_message(detail::Pbtraits<T>::type()));
            auto current = detail::ProtobufMessage(ctx.root());
            // 处理下一级
            detail::EnterProtobufMessageGuard guard(ctx, current);
            for (auto& child : parameter->sorted_children()) {
                child->serialize(out);
            }
            return;
        }

        // 有上级, 需要基于上级创建本级消息 append_message:适用于repeated 消息; mutable_message:适用于普通消息
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();
        auto current = (parent->is_repeated()) ? ctx.append_message(parent, field_name)
                                               : ctx.mutable_message(parent, field_name);
        detail::EnterProtobufMessageGuard guard(ctx, current);
        for (auto& child : parameter->sorted_children()) {
            child->serialize(out);
        }
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.parent_message();

        if (!parent) { // 没有上一级 说明是根
            auto current = detail::ProtobufMessage(ctx.root());
            detail::EnterProtobufMessageGuard guard(ctx, current);
            for (auto& child : parameter->sorted_children()) {
                child->deserialize(in);
            }
            return;
        }

        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();
        auto current = (parent->is_repeated()) ? ctx.mutable_repeated_message(parent, field_name, parent->repeat_index())
                                               : ctx.mutable_message(parent, field_name);

        detail::EnterProtobufMessageGuard guard(ctx, current);
        for (auto& child : parameter->sorted_children()) {
            child->deserialize(in);
        }
    }
};

// SEQUENCE
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_sequence<T>::value || is_set<T>::value>::type> : public Serdes {
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        int repeat_index = 0;
        for (auto& child : parameter->sorted_children()) {
            detail::ProtobufMessage current;

            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            if (!is_stl<typename T::value_type>::value) {
                current = *parent;
                current.set_field_name(field_name); // 传递value_name,用于下一级寻找field
            }

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                current = ctx.append_message(parent, field_name);
                current.set_field_name(detail::Pbtraits<typename T::value_type>::value()); // 传递field_name,用于下一级寻找field
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
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        parameter->mutable_children()->clear();

        for (size_t i = 0; i < ctx.message_field_size(parent, field_name); i++) {
            detail::ProtobufMessage current;

            // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
            if (!is_stl<typename T::value_type>::value) {
                current = *parent;
                current.set_field_name(field_name); // 传递value_name,用于下一级寻找field
            }

            // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
            if (is_stl<typename T::value_type>::value) {
                current = ctx.mutable_repeated_message(parent, field_name, i);
                current.set_field_name(detail::Pbtraits<typename T::value_type>::value()); // 传递field_name,用于下一级寻找field
            }

            // 添加repeat标志
            current.set_repeat_index(i);

            // 处理下一级
            const auto subkey = std::to_string(i);
            detail::EnterProtobufMessageGuard guard(ctx, current);
            auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
            child->deserialize(in);
            parameter->mutable_children()->emplace(subkey, child);
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
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        assert(field && field->is_map());

        auto descriptor = field->message_type();
        auto key_field = descriptor->FindFieldByName("key");
        auto value_field = descriptor->FindFieldByName("value");

        for (auto& child : parameter->sorted_children()) {
            // 添加一个 map 条目
            auto entry = parent->message()->GetReflection()->AddMessage(parent->message(), field);

            // set key
            {
                if (std::is_enum<typename T::key_type>::value) {
                    detail::protobuf_set_value(entry, key_field, child->subkey);
                } else {
                    auto subkey = Converter<typename T::key_type>::from_string(child->subkey);
                    detail::protobuf_set_value(entry, key_field, subkey);
                }
            }

            // set value
            {
                detail::ProtobufMessage current;

                // 创建虚拟的消息层(下一级非STL)
                if (!is_stl<typename T::mapped_type>::value) {
                    current = detail::ProtobufMessage(entry);
                    current.set_field_name("value");
                }

                // 创建真实的消息层(下一级是STL 需要当前层级插入一个真实的消息层)
                if (is_stl<typename T::mapped_type>::value) {
                    auto child_mesg = entry->GetReflection()->MutableMessage(entry, value_field);
                    current = detail::ProtobufMessage(child_mesg);
                    current.set_field_name(detail::Pbtraits<typename T::mapped_type>::value()); // 传递field_name,用于下一级寻找field
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
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        auto field = parent->descriptor()->FindFieldByName(field_name);
        assert(field && field->is_map());

        auto descriptor = field->message_type();
        auto key_field = descriptor->FindFieldByName("key");
        auto value_field = descriptor->FindFieldByName("value");

        parameter->mutable_children()->clear();
        for (size_t i = 0; i < ctx.message_field_size(parent, field_name); i++) {
            const auto& entry = parent->message()->GetReflection()->GetRepeatedMessage(*(parent->message()), field, i);

            // get key
            std::string subkey;
            if (std::is_enum<typename T::key_type>::value) {
                subkey = detail::protobuf_get_value<std::string>(&entry, key_field);
            } else {
                subkey = Converter<typename T::key_type>::to_string(detail::protobuf_get_value<typename T::key_type>(&entry, key_field));
            }

            // get value
            {
                detail::ProtobufMessage current;

                // 创建虚拟的消息层(下一级非STL)
                if (!is_stl<typename T::mapped_type>::value) {
                    current = detail::ProtobufMessage(const_cast<google::protobuf::Message*>(&entry));
                    current.set_field_name("value");
                }

                // 创建真实的消息层(下一级是STL 需要当前层级插入一个真实的消息层)
                if (is_stl<typename T::mapped_type>::value) {
                    auto& child_mesg = entry.GetReflection()->GetMessage(entry, value_field);
                    current = detail::ProtobufMessage(const_cast<google::protobuf::Message*>(&child_mesg));
                    current.set_field_name(detail::Pbtraits<typename T::mapped_type>::value()); // 传递field_name,用于下一级寻找field
                }

                // 处理下一级
                detail::EnterProtobufMessageGuard guard(ctx, current);
                auto child = ParameterPrototype::create_parameter(parameter->detail, subkey, parameter);
                child->deserialize(in);
                parameter->mutable_children()->emplace(subkey, child);
            }
        }
    }
};

// PTR
template <typename T>
class ProtobufSerdes<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Serdes {
public:
private:
    virtual void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto parameter = std::static_pointer_cast<const TraitedParameter<T>>(p);
        auto& ctx = (*static_cast<detail::ProtobufSerdesContext*>(out));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        if (!parameter->value) {
            return;
        }

        detail::ProtobufMessage current;

        // if (is_smart_ptr<typename T::element_type>::value) {
        if (!is_stl<typename T::element_type>::value) {
            current = *parent;
            current.set_field_name(field_name);
        }

        if (is_stl<typename T::element_type>::value) {
            current = ctx.mutable_message(parent, field_name);
            current.set_field_name(detail::Pbtraits<typename T::element_type>::value()); // 传递field_name,用于下一级寻找field
        }

        // 处理下一级
        current.set_repeat_index(-1);
        detail::EnterProtobufMessageGuard guard(ctx, current);
        parameter->value->serialize(out);
    }

    virtual void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        auto parameter = std::static_pointer_cast<TraitedParameter<T>>(p);
        auto& ctx = (*const_cast<detail::ProtobufSerdesContext*>(static_cast<const detail::ProtobufSerdesContext*>(in)));
        auto parent = ctx.parent_message();
        const auto field_name = parent->field_name().empty() ? parameter->subkey : parent->field_name();

        if (!has_field(parent->message(), field_name)) {
            parameter->value = nullptr;
            return;
        }

        if (!parameter->value) {
            parameter->value = ParameterPrototype::create_parameter(parameter->detail, "0", parameter);
        }

        detail::ProtobufMessage current;

        // 创建虚拟的消息层, 拷贝父消息层(下一级非STL)
        if (!is_stl<typename T::element_type>::value) {
            current = *parent;
            current.set_field_name(field_name); // 传递value_name,用于下一级寻找field
        }

        // 创建虚拟的消息层, 构建新的消息层(下一级是STL)
        if (is_stl<typename T::element_type>::value) {
            current = ctx.mutable_message(parent, field_name);
            current.set_field_name(detail::Pbtraits<typename T::element_type>::value()); // 传递field_name,用于下一级寻找field
        }

        // 处理下一级
        current.set_repeat_index(-1);
        detail::EnterProtobufMessageGuard guard(ctx, current);
        parameter->value->deserialize(in);
    }

    bool has_field(google::protobuf::Message* message, const std::string& field_name)
    {
        const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
        const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);

        if (!field) {
            return false;
        }

        const google::protobuf::Reflection* reflection = message->GetReflection();

        // 对于重复字段，检查是否有元素
        if (field->is_repeated()) {
            return reflection->FieldSize(*message, field) > 0;
        }

        // 对于普通字段，使用 HasField
        return reflection->HasField(*message, field);
    }
};

}