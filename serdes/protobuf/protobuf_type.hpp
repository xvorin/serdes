#pragma once

#include "serdes/utils/utils.hpp"

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#include <inttypes.h>
#include <string>

namespace xvorin::serdes::detail {

template <typename T, typename E = void>
struct Pbtraits {
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, double>::value>::type> {
    static std::string type() { return "double"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, float>::value>::type> {
    static std::string type() { return "float"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, int32_t>::value>::type> {
    static std::string type() { return "int32"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, int64_t>::value>::type> {
    static std::string type() { return "int64"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, uint32_t>::value>::type> {
    static std::string type() { return "uint32"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, uint64_t>::value>::type> {
    static std::string type() { return "uint64"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, std::string>::value>::type> {
    static std::string type() { return "string"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_same<T, bool>::value>::type> {
    static std::string type() { return "bool"; }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_enum<T>::value>::type> {
    static std::string type()
    {
        std::vector<std::string> pieces;
        xvorin::serdes::utils::split(Parameter::readable_detail_type(typeid(T)), pieces, "::");
        return pieces.back();
    }
    static std::string value() { return std::string(); }
};

// clang-format off
template <typename T>
struct Pbtraits<T, typename std::enable_if<std::is_integral<T>::value 
        && !std::is_same<T, int32_t>::value 
        && !std::is_same<T, int64_t>::value 
        && !std::is_same<T, uint32_t>::value 
        && !std::is_same<T, uint64_t>::value 
        && !std::is_same<T, bool>::value>::type> {
    static std::string type() { return "int32"; }
     static std::string value() { return std::string(); }
};
// clang-format on

static inline std::string generate_pbtype_name(const std::string& name)
{
    std::vector<std::string> pieces;
    xvorin::serdes::utils::split(name, pieces, "std::");

    std::string type;
    for (auto& r : pieces) {
        for (auto c : r) {
            if (c == '<' || c == '>' || c == ':') {
                c = '_';
            }
            if (c == ',') {
                c = '_';
            }

            auto not_insert = (c == '_' && !type.empty() && type.back() == '_');
            if (!not_insert) {
                type.push_back(c);
            }
        }
    }

    while (!type.empty() && type.back() == '_') {
        type.pop_back();
    }

    type = xvorin::serdes::utils::replace(type, "shared_ptr", "sptr");
    type = xvorin::serdes::utils::replace(type, "unique_ptr", "uptr");

    return type;
}

template <typename T>
struct Pbtraits<T, typename std::enable_if<is_object<T>::value>::type> {
    static std::string type()
    {
        std::vector<std::string> pieces;
        xvorin::serdes::utils::split(Parameter::readable_detail_type(typeid(T)), pieces, "::");
        return pieces.back();
    }
    static std::string value() { return std::string(); }
};

template <typename T>
struct Pbtraits<T, typename std::enable_if<is_stl<T>::value>::type> {
    static std::string type()
    {
        return "T_" + generate_pbtype_name(Parameter::readable_detail_type(typeid(T)));
    }

    static std::string value()
    {
        return "v_" + generate_pbtype_name(Parameter::readable_detail_type(typeid(T)));
    }
};

template <typename T>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, T value)
{
    msg->GetReflection()->SetInt32(msg, field, static_cast<int32_t>(value));
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int32_t value)
{
    msg->GetReflection()->SetInt32(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int64_t value)
{
    msg->GetReflection()->SetInt64(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, uint32_t value)
{
    msg->GetReflection()->SetUInt32(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, uint64_t value)
{
    msg->GetReflection()->SetUInt64(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, double value)
{
    msg->GetReflection()->SetDouble(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, float value)
{
    msg->GetReflection()->SetFloat(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, std::string value)
{
    msg->GetReflection()->SetString(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, bool value)
{
    msg->GetReflection()->SetBool(msg, field, value);
}

template <>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, const google::protobuf::EnumValueDescriptor* value)
{
    msg->GetReflection()->SetEnum(msg, field, value);
}

template <typename T>
inline T protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return static_cast<T>(msg->GetReflection()->GetInt32(*msg, field));
}

template <>
inline int32_t protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetInt32(*msg, field);
}

template <>
inline int64_t protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetInt64(*msg, field);
}

template <>
inline uint32_t protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetUInt32(*msg, field);
}

template <>
inline uint64_t protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetUInt64(*msg, field);
}

template <>
inline double protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetDouble(*msg, field);
}

template <>
inline float protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetFloat(*msg, field);
}

template <>
inline std::string protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetString(*msg, field);
}

template <>
inline bool protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetBool(*msg, field);
}

template <>
inline const google::protobuf::EnumValueDescriptor* protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetEnum(*msg, field);
}

template <typename T>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, T value)
{
    msg->GetReflection()->SetRepeatedInt32(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, int32_t value)
{
    msg->GetReflection()->SetRepeatedInt32(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, int64_t value)
{
    msg->GetReflection()->SetRepeatedInt64(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, uint32_t value)
{
    msg->GetReflection()->SetRepeatedUInt32(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, uint64_t value)
{
    msg->GetReflection()->SetRepeatedUInt64(msg, field, index, value);
}
template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, double value)
{
    msg->GetReflection()->SetRepeatedDouble(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, float value)
{
    msg->GetReflection()->SetRepeatedFloat(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, std::string value)
{
    msg->GetReflection()->SetRepeatedString(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, bool value)
{
    msg->GetReflection()->SetRepeatedBool(msg, field, index, value);
}

template <>
inline void protobuf_set_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index, const google::protobuf::EnumValueDescriptor* value)
{
    msg->GetReflection()->SetRepeatedEnum(msg, field, index, value);
}

template <typename T>
inline T protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedInt32(*msg, field, index);
}

template <>
inline int32_t protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedInt32(*msg, field, index);
}

template <>
inline int64_t protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedInt64(*msg, field, index);
}

template <>
inline uint32_t protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedUInt32(*msg, field, index);
}

template <>
inline uint64_t protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedUInt64(*msg, field, index);
}

template <>
inline double protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedDouble(*msg, field, index);
}

template <>
inline float protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedFloat(*msg, field, index);
}

template <>
inline std::string protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedString(*msg, field, index);
}

template <>
inline bool protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedBool(*msg, field, index);
}

template <>
inline const google::protobuf::EnumValueDescriptor* protobuf_get_repeated_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int index)
{
    return msg->GetReflection()->GetRepeatedEnum(*msg, field, index);
}

template <typename T>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, T value)
{
    msg->GetReflection()->AddInt32(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int32_t value)
{
    msg->GetReflection()->AddInt32(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, int64_t value)
{
    msg->GetReflection()->AddInt64(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, uint32_t value)
{
    msg->GetReflection()->AddUInt32(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, uint64_t value)
{
    msg->GetReflection()->AddUInt64(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, double value)
{
    msg->GetReflection()->AddDouble(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, float value)
{
    msg->GetReflection()->AddFloat(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, std::string value)
{
    msg->GetReflection()->AddString(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, bool value)
{
    msg->GetReflection()->AddBool(msg, field, value);
}

template <>
inline void protobuf_add_repeated_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, const google::protobuf::EnumValueDescriptor* value)
{
    msg->GetReflection()->AddEnum(msg, field, value);
}

}