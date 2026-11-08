#pragma once

#include "serdes/serdes/config.h"
#include "serdes/utils/file_monitor.hpp"
#include "serdes/utils/utils.hpp" // utils::read_file / utils::write_file_atomic

#include <string>

namespace xvorin::serdes {

// 参数树的数据载体抽象: 子树内容的来源/去处, 也是变更事件源。
// 文件只是实现之一(FileSink); 后续可扩展 MemorySink/HttpSink/EtcdSink 等, 参数树侧不感知。
class ParameterSink {
public:
    virtual ~ParameterSink() = default;

    // 读取内容文本; 失败抛 BadFile/EnvarNotExist 等标准异常
    virtual std::string read() = 0;
    // 写入内容文本; 实现保证原子性, 失败抛 BadFile
    virtual void write(const std::string& content) = 0;

    // 标识: 文件路径 / URL / key(外部格式的字符串形态)
    virtual const std::string& id() const = 0;
    // 内容格式: 文件按扩展名, 网络按 content-type 等; 无法确定时返回 PST_STRU(由调用方回退)
    virtual ParameterSerdesType format() const = 0;

    // 订阅内容变更: 返回 RAII 句柄(析构自动取消); 不支持监听可返回 nullptr
    virtual std::shared_ptr<Monitor> watch(const std::string& entity, const std::string& param, std::function<void()> on_change) = 0;
};

// 本地文件实现: 读(read_file, ${VAR} 展开) / 原子写 / 扩展名推断格式 / 共享文件监听订阅
class FileSink : public ParameterSink {
public:
    // fmt_override 非 PST_STRU 时作为格式(供 set_sink_file 显式指定覆盖扩展名推断), 否则按扩展名
    explicit FileSink(std::string path, ParameterSerdesType fmt_override = ParameterSerdesType::PST_STRU)
        : path_(std::move(path))
        , fmt_override_(fmt_override)
    {
    }

    std::string read() override
    {
        return utils::read_file(path_);
    }

    void write(const std::string& content) override
    {
        utils::write_file_atomic(path_, content);
    }

    const std::string& id() const override
    {
        return path_;
    }

    ParameterSerdesType format() const override
    {
        if (fmt_override_ != ParameterSerdesType::PST_STRU) {
            return fmt_override_; // set_sink_file 显式指定(覆盖扩展名)
        }
        // 按扩展名推断; 无扩展名/未知返回 PST_STRU
        const auto dot = path_.find_last_of('.');
        if (dot == std::string::npos) {
            return ParameterSerdesType::PST_STRU;
        }
        const std::string ext = path_.substr(dot + 1);
#if ENABLE_JSON
        if (ext == "json") {
            return ParameterSerdesType::PST_JSON;
        }
#endif
#if ENABLE_YAML
        if (ext == "yaml" || ext == "yml") {
            return ParameterSerdesType::PST_YAML;
        }
#endif
#if ENABLE_TOML
        if (ext == "toml") {
            return ParameterSerdesType::PST_TOML;
        }
#endif
#if ENABLE_XML
        if (ext == "xml") {
            return ParameterSerdesType::PST_XML;
        }
#endif
#if ENABLE_PROTOBUF
        if (ext == "pbtxt") {
            return ParameterSerdesType::PST_PBFMT; // protobuf 文本格式(子配置)
        }
        if (ext == "pbbin") {
            return ParameterSerdesType::PST_PBBIN; // protobuf 二进制格式(子配置)
        }
#endif
        return ParameterSerdesType::PST_STRU;
    }

    std::shared_ptr<Monitor> watch(const std::string& entity,
        const std::string& param, std::function<void()> on_change) override
    {
        // 与读写一致: 路径里的 ${VAR} 先展开再监听
        const auto expanded = static_cast<std::string>(envar(path_));
        return FileMonitor::instance().monitor(entity, param, expanded, std::move(on_change));
    }

private:
    std::string path_;
    ParameterSerdesType fmt_override_ = ParameterSerdesType::PST_STRU;
};

}
