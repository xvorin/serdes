#pragma once

// 各格式后端开关: 业务库可用 -DENABLE_XXX=0 关闭对应后端
#ifndef ENABLE_JSON
#define ENABLE_JSON 1
#endif

#ifndef ENABLE_YAML
#define ENABLE_YAML 1
#endif

#ifndef ENABLE_TOML
#define ENABLE_TOML 1
#endif

#ifndef ENABLE_XML
#define ENABLE_XML 1
#endif

// 业务库可用 -DENABLE_PROTOBUF=0 关掉 protobuf 后端，从而不必链接 libprotobuf
#ifndef ENABLE_PROTOBUF
#define ENABLE_PROTOBUF 0
#endif

namespace xvorin::serdes {

// 参数序列化方式
enum ParameterSerdesType {
    PST_STRU,
#if ENABLE_JSON
    PST_JSON,
#endif

#if ENABLE_YAML
    PST_YAML,
#endif

#if ENABLE_TOML
    PST_TOML,
#endif

#if ENABLE_XML
    PST_XML,
#endif

#if ENABLE_PROTOBUF
    PST_PBDEF,
    PST_PBFMT,
#endif
};

}

#include "serdes/serdes/struct_serdes.hpp"

#if ENABLE_JSON
#include "serdes/serdes/json/json_serdes.hpp"
#endif

#if ENABLE_YAML
#include "serdes/serdes/yaml/yaml_serdes.hpp"
#endif

#if ENABLE_TOML
#include "serdes/serdes/toml/toml_serdes.hpp"
#endif

#if ENABLE_XML
#include "serdes/serdes/xml/xml_serdes.hpp"
#endif

#if ENABLE_PROTOBUF
#include "serdes/serdes/protobuf/protobuf_define_serdes.hpp"
#include "serdes/serdes/protobuf/protobuf_serdes.hpp"
#endif

namespace xvorin::serdes {

template <typename T>
static inline std::map<ParameterSerdesType, std::unique_ptr<Serdes>> make_serdess()
{
    std::map<ParameterSerdesType, std::unique_ptr<Serdes>> serdess;

#if ENABLE_JSON
    serdess[ParameterSerdesType::PST_JSON] = std::unique_ptr<JsonSerdes<T>>(new JsonSerdes<T>());
#endif

#if ENABLE_YAML
    serdess[ParameterSerdesType::PST_YAML] = std::unique_ptr<YamlSerdes<T>>(new YamlSerdes<T>());
#endif

#if ENABLE_TOML
    serdess[ParameterSerdesType::PST_TOML] = std::unique_ptr<TomlSerdes<T>>(new TomlSerdes<T>());
#endif

#if ENABLE_XML
    serdess[ParameterSerdesType::PST_XML] = std::unique_ptr<XmlSerdes<T>>(new XmlSerdes<T>());
#endif

#if ENABLE_PROTOBUF
    serdess[ParameterSerdesType::PST_PBDEF] = std::unique_ptr<ProtobufDefineSerdes<T>>(new ProtobufDefineSerdes<T>());
    serdess[ParameterSerdesType::PST_PBFMT] = std::unique_ptr<ProtobufSerdes<T>>(new ProtobufSerdes<T>());
#endif

    return serdess;
}

}