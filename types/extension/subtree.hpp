#pragma once

#include <string>

namespace xvorin::serdes {

// 配置引用: 继承被引用配置类型 T, 结构体上的成员可直接访问(cfg.camera.width), 另带一个配置文件路径;
// 反序列化时把该文件内容以对象子树的形式挂入参数树, 与结构体成员双向同步。
// T 须是可通过 DEFINE_PARAM 注册成员的结构体。
// 外部序列化格式(JSON/YAML/TOML/XML)始终为路径字符串。
// 文件读取与 ${VAR} 展开见 utils/utils.hpp 的 read_file。
template <typename T>
struct subtree : public T {
    using value_type = T; // 被引用配置类型: 序列化按该类型的成员偏移读写

    // 被引用配置本体(需要以整体取用/比较时用); 注意 T 内若有名为 value 的成员会被本接口遮蔽
    T& value()
    {
        return *this;
    }
    const T& value() const
    {
        return *this;
    }

    // 配置文件路径(引用路径); 名字刻意避开常见成员名, 避免与 T 的成员冲突
    std::string _subtree_sink_path_;
};

}
