#pragma once

// 后端开关 + 参数序列化类型。
// 放在最底层是刻意的: types/ 与各实现头都只需要这个枚举, 不应为此依赖 config.h
// (config.h 会带入 struct_serdes 与全部后端, 反向依赖类型层)。

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
    PST_PBFMT, // protobuf 文本(pbtxt)
    PST_PBBIN, // protobuf 二进制(pbbin)
#endif
};

// 默认配置文件格式: 优先toml(可保存注释), 依次回退到其他已启用的后端, 避免关闭某后端后引用不存在的枚举值
inline ParameterSerdesType default_sink_type()
{
#if ENABLE_TOML
    return ParameterSerdesType::PST_TOML;
#elif ENABLE_JSON
    return ParameterSerdesType::PST_JSON;
#elif ENABLE_YAML
    return ParameterSerdesType::PST_YAML;
#elif ENABLE_XML
    return ParameterSerdesType::PST_XML;
#else
    return ParameterSerdesType::PST_STRU;
#endif
}

}
