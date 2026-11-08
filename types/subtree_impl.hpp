#pragma once

#include "serdes/serdes/config.h"
#include "serdes/serdes/multi_serdes.hpp"
#include "serdes/serdes/parameter_sink.hpp"
#include "serdes/types/composite.hpp"
#include "serdes/types/extension/subtree.hpp"
#include "serdes/types/prototype.hpp"
#include "serdes/types/subtree.hpp"
#include "serdes/types/traits.hpp"

#include <sstream>

namespace xvorin::serdes {

// subtree 的参数节点: 结构上等价于一个以 T 为类型的对象子树, 额外携带配置文件路径。
// 子节点在 clone 时从 T 的原型嫁接, 保证整棵树创建时即可看到被引用配置的默认结构。
template <typename T>
struct TraitedParameter<subtree<T>, typename std::enable_if<is_subtree<subtree<T>>::value>::type> : public Composite<subtree<T>>, public Subtree {
    using subtype = subtree<T>;

    TraitedParameter(std::string subkey, size_t offset, std::string comment,
        typename Composite<subtree<T>>::informant inform, std::string verinfo)
        : Composite<subtree<T>>(std::move(subkey), typeid(subtype), offset, std::move(comment), std::move(inform), std::move(verinfo))
    {
        this->sort_children_strategy = sort_parameter_by_offset;
        subtree_serdes_ = std::make_shared<MultiSerdes<T>>(*this); // 按被引用类型 T 做内容读写
    }

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override
    {
        auto cloned = std::make_shared<TraitedParameter<subtree<T>>>(std::move(newkey), newoffset, this->comment, this->inform, this->verinfo);
        cloned->parent = this->parent;
        cloned->path = this->path;

        // 从被引用类型的原型重建子节点: 用 clone 保留各子节点自身类型
        // (序列/映射节点的 detail 是其元素类型, 不能按 detail 重建)
        auto cproto = ParameterPrototype::create_parameter(typeid(T));
        for (const auto& child_pair : cproto->children()) {
            const auto& ochild = child_pair.second;
            auto nchild = ochild->clone(ochild->subkey, ochild->offset);
            nchild->parent = cloned;
            cloned->mutable_children()->emplace(nchild->subkey, nchild);
        }
        return cloned;
    }

    virtual void create_child(const std::string& newkey) override
    {
        auto children = this->mutable_children();
        if (children->find(newkey) != children->end()) {
            throw IndexDuplicate(this->index() + "." + newkey);
        }

        auto temp = ParameterPrototype::create_parameter(typeid(T));
        auto child = temp->find_child(newkey);
        if (nullptr == child) {
            throw ParameterNotFound(newkey + "@" + this->index());
        }

        children->emplace(newkey, child);
        child->parent = this->shared_from_this();
    }

    virtual void remove_child(const std::string& subkey) override
    {
        auto iter = this->children().find(subkey);
        if (iter == this->children().end()) {
            throw ParameterNotFound(this->index() + "." + subkey);
        }
        this->mutable_children()->erase(iter);
    }

    // 直接以字符串设置引用路径(set/命令行 --x=path); 内容在下次 load/deserialize 时重新加载
    virtual void from_string(const std::string& in) override
    {
        path = in;
    }

    // 将子树当前内容经载体按指定格式写回; 嵌套 subtree 在内容序列化过程中会各自触发 save
    virtual void save(ParameterSerdesType type) const override
    {
        if (!path.empty()) {
            auto target = sink ? sink : std::make_shared<FileSink>(path); // 未注入则用默认文件载体

            // 格式按载体推断(扩展名), 无法确定时回退主格式
            auto fmt = target->format();
            if (fmt == ParameterSerdesType::PST_STRU) {
                fmt = type;
            }

            // 序列化期间按 fmt 临时切 profile 类型, 让内容走对应后端
            auto prof = this->profile();
            const auto saved_type = prof->serdes_type;
            prof->serdes_type = fmt;

            std::string content;
            bool write_content = false;
#if ENABLE_JSON
            if (fmt == ParameterSerdesType::PST_JSON) {
                nlohmann::ordered_json jout;
                subtree_serdes_->serialize(this->shared_from_this(), &jout);
                content = json::dump(jout);
                write_content = !content.empty();
            }
#endif
#if ENABLE_YAML
            if (fmt == ParameterSerdesType::PST_YAML) {
                fkyaml::ordered_node yout = fkyaml::ordered_node::mapping();
                subtree_serdes_->serialize(this->shared_from_this(), &yout);
                content = yaml::dump(yout);
                write_content = !content.empty();
            }
#endif
#if ENABLE_TOML
            if (fmt == ParameterSerdesType::PST_TOML) {
                ::toml::ordered_value tout = ::toml::ordered_table();
                subtree_serdes_->serialize(this->shared_from_this(), &tout);
                content = toml::dump(tout);
                write_content = !content.empty();
            }
#endif
#if ENABLE_XML
            if (fmt == ParameterSerdesType::PST_XML) {
                pugi::xml_document xdoc;
                auto xroot = xml_append_element(xdoc, "subtree");
                subtree_serdes_->serialize(this->shared_from_this(), &xroot);
                content = xml::dump(xdoc);
                write_content = !content.empty();
            }
#endif
#if ENABLE_PROTOBUF
            if (fmt == ParameterSerdesType::PST_PBFMT || fmt == ParameterSerdesType::PST_PBBIN) {
                // 子文件以 protobuf 承载 C 的对象内容: 先按 PBDEF 生成 C 的 schema, 再输出 pbtxt/pbbin
                prof->serdes_type = ParameterSerdesType::PST_PBDEF;
                detail::ProtobufDefineSerdesContext def;
                subtree_serdes_->serialize(this->shared_from_this(), &def);

                const auto package = detail::extract_pbpackage_name(Parameter::readable_detail_type(typeid(T)));
                auto factory = std::make_shared<detail::ProtobufMessageFactory>(package, def.to_string(package));
                prof->serdes_type = ParameterSerdesType::PST_PBFMT;

                detail::ProtobufSerdesContext ctx(factory);
                subtree_serdes_->serialize(this->shared_from_this(), &ctx);

                const bool binary = (fmt == ParameterSerdesType::PST_PBBIN);
                content = binary ? ctx.to_binary_string() : ctx.to_txt_string();
                write_content = binary || !content.empty(); // 空二进制串也是合法内容
            }
#endif
            // 其余格式(如 PBDEF: proto 定义)没有子文件内容的含义, write_content 保持 false
            prof->serdes_type = saved_type;

            if (write_content) {
                target->write(content);
                // 记录最近写回内容, 供监控自激抑制(load 读到相同内容时跳过)
                this->sink_content = content;
            }
        }
    }

    // 按当前 path 重载内容(经载体读取; 扩展名推断, 无扩展名回退到 type); 由树的线程安全入口调用
    void load(ParameterSerdesType type) override
    {
        if (path.empty()) {
            return;
        }
        auto target = sink ? sink : std::make_shared<FileSink>(path); // 未注入则用默认文件载体

        auto fmt = target->format();
        if (fmt == ParameterSerdesType::PST_STRU) {
            fmt = type;
        }

        auto prof = this->profile();
        const auto saved_type = prof->serdes_type;
        prof->serdes_type = fmt;

        const auto content = target->read();

        // 自激抑制: 内容与最近一次写回一致(如 save 刚写回触发的监控事件)时无需重载
        if (content == this->sink_content) {
            prof->serdes_type = saved_type;
            return;
        }
        switch (fmt) {
#if ENABLE_JSON
        case ParameterSerdesType::PST_JSON: {
            auto jfile = json::parse(content);
            subtree_serdes_->deserialize(this->shared_from_this(), &jfile);
            break;
        }
#endif
#if ENABLE_YAML
        case ParameterSerdesType::PST_YAML: {
            auto yfile = yaml::parse(content);
            subtree_serdes_->deserialize(this->shared_from_this(), &yfile);
            break;
        }
#endif
#if ENABLE_TOML
        case ParameterSerdesType::PST_TOML: {
            auto tfile = toml::parse(content);
            subtree_serdes_->deserialize(this->shared_from_this(), &tfile);
            break;
        }
#endif
#if ENABLE_XML
        case ParameterSerdesType::PST_XML: {
            pugi::xml_document xdoc = xml::parse(content);
            auto xroot = xdoc.document_element();
            subtree_serdes_->deserialize(this->shared_from_this(), &xroot);
            break;
        }
#endif
#if ENABLE_PROTOBUF
        case ParameterSerdesType::PST_PBFMT:
        case ParameterSerdesType::PST_PBBIN: {
            // protobuf 子文件: 以 C 的对象内容读入(先按 PBDEF 建 schema)
            prof->serdes_type = ParameterSerdesType::PST_PBDEF;
            detail::ProtobufDefineSerdesContext def;
            subtree_serdes_->serialize(this->shared_from_this(), &def);

            const auto package = detail::extract_pbpackage_name(Parameter::readable_detail_type(typeid(T)));
            auto factory = std::make_shared<detail::ProtobufMessageFactory>(package, def.to_string(package));
            prof->serdes_type = ParameterSerdesType::PST_PBFMT;

            detail::ProtobufSerdesContext ctx(factory);
            if (fmt == ParameterSerdesType::PST_PBBIN) {
                if (!ctx.from_binary_string(detail::Pbtraits<T>::type(), content)) {
                    throw ParsePbbinException(target->id());
                }
            } else if (!ctx.from_txt_string(detail::Pbtraits<T>::type(), content)) {
                throw ParsePbtxtException(target->id());
            }
            subtree_serdes_->deserialize(this->shared_from_this(), &ctx);
            break;
        }
#endif
        default:
            break; // 其余格式(如 PBDEF)不参与子树文件读写
        }

        // 加载成功后才更新基线: 之后 save 写回同内容可被自激抑制
        this->sink_content = content;
        prof->serdes_type = saved_type;
    }

    virtual std::string debug_self() const override
    {
        std::stringstream ss;
        ss << " size = " << this->children().size() << " path = " << path << " [" + this->readable_detail_type() + "]";
        if (!this->comment.empty()) {
            ss << " #" << this->comment;
        }
        return ss.str();
    }

private:
    // 内容读写: 按被引用类型 T 实例化(不含 subtree 的路径语义), save/load 走这个而非外层格式的 serdes_
    std::shared_ptr<MultiSerdes<T>> subtree_serdes_;
};

}
