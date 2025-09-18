#pragma once

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#include <inttypes.h>
#include <string>

namespace xvorin::serdes {

template <typename T>
inline std::string protobuf_type()
{
    return "int32";
}

template <>
inline std::string protobuf_type<double>()
{
    return "double";
}

template <>
inline std::string protobuf_type<float>()
{
    return "float";
}

template <>
inline std::string protobuf_type<int32_t>()
{
    return "int32";
}

template <>
inline std::string protobuf_type<int64_t>()
{
    return "int64";
}

template <>
inline std::string protobuf_type<uint32_t>()
{
    return "uint32";
}

template <>
inline std::string protobuf_type<uint64_t>()
{
    return "uint64";
}

template <>
inline std::string protobuf_type<std::string>()
{
    return "bytes";
}

template <>
inline std::string protobuf_type<bool>()
{
    return "bool";
}

namespace detail {

template <typename T>
inline void protobuf_set_value(google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field, T value)
{
    msg->GetReflection()->SetInt32(msg, field, value);
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

template <typename T>
inline T protobuf_get_value(const google::protobuf::Message* msg, const google::protobuf::FieldDescriptor* field)
{
    return msg->GetReflection()->GetInt32(*msg, field);
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

inline void split(const std::string& str, std::vector<std::string>& result, const std::string& delim = " ")
{
    if (str.empty() || delim.empty()) {
        return;
    }

    std::string::const_iterator substart = str.begin();
    std::string::const_iterator subend = substart;
    while (true) {
        subend = std::search(substart, str.end(), delim.begin(), delim.end());
        std::string temp(substart, subend);

        if (!temp.empty()) {
            result.push_back(temp);
        }

        if (subend == str.end()) {
            break;
        }
        substart = subend + delim.size();
    }
}

inline std::string simplify_message_name(const std::string& name)
{
    std::vector<std::string> result;
    split(name, result, "::");
    return result.empty() ? "" : result.back();
}

inline void make_internal_name(const std::string& name, std::string& type, std::string& value)
{
    std::vector<std::string> result;
    split(name, result, "std::");

    std::string keyword;
    for (auto& r : result) {
        for (auto c : r) {
            if (c == '<' || c == '>' || c == ':') {
                c = '_';
            }
            if (c == ',') {
                c = '_';
            }

            auto not_insert = (c == '_' && !keyword.empty() && keyword.back() == '_');
            if (!not_insert) {
                keyword.push_back(c);
            }
        }
    }

    while (!keyword.empty() && keyword.back() == '_') {
        keyword.pop_back();
    }

    type = "T_" + keyword;
    value = "v_" + keyword;
}

}
}