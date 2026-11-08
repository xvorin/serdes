#pragma once

#include "serdes/serdes/config.h"

#include <memory>
#include <string>

namespace xvorin::serdes {

class ParameterSink;

// subtree 节点向外部暴露的持久化/监控接口:
//   save(type)     把自己内容写回载体; 保存主配置时由各后端的 subtree 序列化触发(见 profile.subtree_sink)
//   load(type)     按当前 path 重载内容(监控回调经树入口调用, 见 ParameterTree::reload_subtree)
//   path           引用路径(配置文件路径/URL/key, 也是外部格式里持久化的那个字符串)
//   sink           注入的数据载体; 为空时按 path 构造 FileSink(直接赋值可替换为内存/网络等实现)
//   sink_content    最近一次写回/加载的内容快照, 供监控自激抑制(load 读到相同内容时跳过重载);
//                   与 CommandLineAPI::sink_content_ 的语义一致
class Subtree {
public:
    virtual ~Subtree() = default;
    // 把自身内容写回载体(含嵌套 subtree: 内容序列化时会自然触发子节点的 save)
    virtual void save(ParameterSerdesType type) const = 0;
    virtual void load(ParameterSerdesType type) = 0;

public:
    std::string path; // 引用路径(配置文件路径/URL/key)
    std::shared_ptr<ParameterSink> sink; // 为空表示用默认的 FileSink(path)
    mutable std::string sink_content; // 自激抑制基线: 最近一次写回/成功加载的内容(save 为 const, 故 mutable)
};

}
