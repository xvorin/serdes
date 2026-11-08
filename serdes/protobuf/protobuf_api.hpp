#include "serdes/serdes/protobuf/protobuf_serdes.hpp"
#include "serdes/types/parameter_tree.hpp"

namespace xvorin::serdes {

template <typename T>
class ProtobufAPI {
public:
    explicit ProtobufAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
        // 未手动指定 package 时, 自动从根类型的命名空间推导 (ihs::evdet::InputFrame -> ihs.evdet)
        // 之后调用 set_package 可覆盖此默认值
        package_ = detail::extract_pbpackage_name(Parameter::readable_detail_type(typeid(T)));

        // 预生成 subtree_inline 两种模式下的 schema: 之后切换开关只换指针, 不再重建描述符
        build_schema(plain_, false);
        build_schema(inline_, true);
        if (inline_.pbdef == plain_.pbdef) { // 树中没有 subtree 时两种模式等价, 复用描述符池
            inline_.factory = plain_.factory;
            inline_.ctx.reset(new detail::ProtobufSerdesContext(inline_.factory));
        }
    }

    void set_package(std::string package);
    const std::string& package();

    void from_pbbin(const std::string& s);
    void from_pbbin(const std::string& index, const std::string& s);
    void from_pbbin(const std::string& s, T* value);
    void from_pbbin(const std::string& index, const std::string& s, T* value);

    void from_pbtxt(const std::string& s);
    void from_pbtxt(const std::string& index, const std::string& s);
    void from_pbtxt(const std::string& s, T* value);
    void from_pbtxt(const std::string& index, const std::string& s, T* value);

    std::string to_pbbin();
    std::string to_pbbin(const std::string& index);
    std::string to_pbbin(const T& value);
    std::string to_pbbin(const T& value, const std::string& index);

    std::string to_pbtxt();
    std::string to_pbtxt(const std::string& index);
    std::string to_pbtxt(const T& value);
    std::string to_pbtxt(const T& value, const std::string& index);

    std::string to_pbdef();
    std::string to_pbdef(const std::string& index);

    std::string to_pbdbstr(bool simplified = false);
    std::string to_pbdbstr(const std::string& index, bool simplified = false);
    std::string to_pbdbstr(const T& value, bool simplified = false);
    std::string to_pbdbstr(const T& value, const std::string& index, bool simplified = false);

private:
    // 一套动态 schema: pbdef 文本 + 描述符池 + 复用的序列化 context(持有 Message 对象)
    struct Schema {
        std::string pbdef;
        std::shared_ptr<detail::ProtobufMessageFactory> factory;
        std::unique_ptr<detail::ProtobufSerdesContext> ctx;
    };

    /// 按指定 subtree_inline 生成一套 schema(内部临时切 profile 开关, 生成后恢复)
    void build_schema(Schema& schema, bool subtree_inline);
    /// 当前生效的 schema(由 profile 的 subtree_inline 决定)
    Schema& schema();

private:
    ParameterTree<T>& tree_;
    std::string package_;
    Schema plain_; // subtree_inline = false: subtree 字段为 string(路径)
    Schema inline_; // subtree_inline = true: subtree 字段为嵌套 message
};

template <typename T>
void ProtobufAPI<T>::set_package(std::string package)
{
    if (package_ != package) {
        package_.swap(package);
        build_schema(plain_, false);
        build_schema(inline_, true);
        if (inline_.pbdef == plain_.pbdef) {
            inline_.factory = plain_.factory;
            inline_.ctx.reset(new detail::ProtobufSerdesContext(inline_.factory));
        }
    }
}

template <typename T>
const std::string& ProtobufAPI<T>::package()
{
    return package_;
}

template <typename T>
void ProtobufAPI<T>::build_schema(Schema& schema, bool subtree_inline)
{
    // pbdef 生成依赖 subtree_inline: 经树的序列化闸门临时切换后恢复, 避免与并发序列化竞争
    const bool saved = tree_.subtree_inline();
    tree_.enable_subtree_inline(subtree_inline);
    schema.pbdef = to_pbdef(tree_.root());
    tree_.enable_subtree_inline(saved);

    schema.factory = std::make_shared<detail::ProtobufMessageFactory>(package_, schema.pbdef);
    schema.ctx.reset(new detail::ProtobufSerdesContext(schema.factory));
}

template <typename T>
typename ProtobufAPI<T>::Schema& ProtobufAPI<T>::schema()
{
    return tree_.parameter(tree_.root())->profile()->subtree_inline ? inline_ : plain_;
}

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& s)
{
    from_pbbin(tree_.root(), s);
}

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& index, const std::string& s)
{
    from_pbbin(index, s, tree_.unsafe_value());
}

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& s, T* value)
{
    from_pbbin(tree_.root(), s, value);
}

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& index, const std::string& s, T* value)
{
    auto p = tree_.parameter(index);

    schema().ctx->from_binary_string(detail::extract_pbmessage_name(p->readable_detail_type()), s);

    tree_.deserialize(p, schema().ctx.get(), ParameterSerdesType::PST_PBFMT);
    tree_.commit_model_changes(value);
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& s)
{
    from_pbtxt(tree_.root(), s);
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& index, const std::string& s)
{
    from_pbtxt(index, s, tree_.unsafe_value());
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& s, T* value)
{
    from_pbtxt(tree_.root(), s, value);
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& index, const std::string& s, T* value)
{
    auto p = tree_.parameter(index);

    schema().ctx->from_txt_string(detail::extract_pbmessage_name(p->readable_detail_type()), s);

    tree_.deserialize(p, schema().ctx.get(), ParameterSerdesType::PST_PBFMT);
    tree_.commit_model_changes(value);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin()
{
    return to_pbbin(tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin(const std::string& index)
{
    tree_.commit_value_changes();

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_binary_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin(const T& value)
{
    return to_pbbin(value, tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin(const T& value, const std::string& index)
{
    tree_.commit_value_changes(value);

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_binary_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt()
{
    return to_pbtxt(tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt(const std::string& index)
{
    tree_.commit_value_changes();

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_txt_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt(const T& value)
{
    return to_pbtxt(value, tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt(const T& value, const std::string& index)
{
    tree_.commit_value_changes(value);

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_txt_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdef()
{
    return schema().pbdef;
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdef(const std::string& index)
{
    tree_.commit_value_changes();

    detail::ProtobufDefineSerdesContext out;
    tree_.serialize(tree_.parameter(index), &out, ParameterSerdesType::PST_PBDEF);

    return out.to_string(package_);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(bool simplified)
{
    return to_pbdbstr(tree_.root(), simplified);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(const std::string& index, bool simplified)
{
    tree_.commit_value_changes();

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_debug_string(simplified);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(const T& value, bool simplified)
{
    return to_pbdbstr(value, tree_.root(), simplified);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(const T& value, const std::string& index, bool simplified)
{
    tree_.commit_value_changes(value);

    tree_.serialize(tree_.parameter(index), schema().ctx.get(), ParameterSerdesType::PST_PBFMT);

    return schema().ctx->to_debug_string(simplified);
}
}