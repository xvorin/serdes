#include "serdes/types/parameter_tree.hpp"

#include "serdes/serdes/protobuf/protobuf_serdes.hpp"

namespace xvorin::serdes {

template <typename T>
class ProtobufAPI {
public:
    explicit ProtobufAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    void from_pbbin(const std::string& s);
    void from_pbbin(const std::string& index, const std::string& s);

    void from_pbtxt(const std::string& s);
    void from_pbtxt(const std::string& index, const std::string& s);

    std::string to_pbbin();
    std::string to_pbbin(const std::string& index);

    std::string to_pbtxt();
    std::string to_pbtxt(const std::string& index);

    std::string to_pbdef(const std::string& package);
    std::string to_pbdef_internal(const std::string& index, const std::string& package = "");

    std::string to_pbdbstr(bool simplified = false);
    std::string to_pbdbstr(const std::string& index, bool simplified = false);

private:
    ParameterTree<T>& tree_;
};

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& s)
{
    from_pbbin(tree_.root(), s);
}

template <typename T>
void ProtobufAPI<T>::from_pbbin(const std::string& index, const std::string& s)
{
    auto pbdef = to_pbdef_internal(index);
    auto p = tree_.parameter(index);

    detail::ProtobufSerdesContext pbbin_in(pbdef);
    pbbin_in.from_binary_string(detail::generate_pbtype_name(p->readable_detail_type()), s);

    tree_.deserialize(p, &pbbin_in, ParameterSerdesType::PST_PBFMT);
    tree_.commit_model_changes();
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& s)
{
    from_pbtxt(tree_.root(), s);
}

template <typename T>
void ProtobufAPI<T>::from_pbtxt(const std::string& index, const std::string& s)
{
    auto pbdef = to_pbdef_internal(index);
    auto p = tree_.parameter(index);

    detail::ProtobufSerdesContext pbtxt_in(pbdef);
    pbtxt_in.from_txt_string(detail::generate_pbtype_name(p->readable_detail_type()), s);

    tree_.deserialize(p, &pbtxt_in, ParameterSerdesType::PST_PBFMT);
    tree_.commit_model_changes();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin()
{
    return to_pbbin(tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbbin(const std::string& index)
{
    auto pbdef = to_pbdef_internal(index);

    detail::ProtobufSerdesContext pbbin_out(pbdef);
    tree_.serialize(tree_.parameter(index), &pbbin_out, ParameterSerdesType::PST_PBFMT);

    return pbbin_out.to_binary_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt()
{
    return to_pbtxt(tree_.root());
}

template <typename T>
std::string ProtobufAPI<T>::to_pbtxt(const std::string& index)
{
    auto pbdef = to_pbdef_internal(index);

    detail::ProtobufSerdesContext pbtxt_out(pbdef);
    tree_.serialize(tree_.parameter(index), &pbtxt_out, ParameterSerdesType::PST_PBFMT);

    return pbtxt_out.to_txt_string();
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdef(const std::string& package)
{
    return to_pbdef_internal(tree_.root(), package);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdef_internal(const std::string& index, const std::string& package)
{
    tree_.commit_value_changes();

    detail::ProtobufDefineSerdesContext out;
    tree_.serialize(tree_.parameter(index), &out, ParameterSerdesType::PST_PBDEF);

    return out.to_string(package);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(bool simplified)
{
    return to_pbdbstr(tree_.root(), simplified);
}

template <typename T>
std::string ProtobufAPI<T>::to_pbdbstr(const std::string& index, bool simplified)
{
    auto pbdef = to_pbdef_internal(index);

    detail::ProtobufSerdesContext pbdbstr_out(pbdef);
    tree_.serialize(tree_.parameter(index), &pbdbstr_out, ParameterSerdesType::PST_PBFMT);

    return pbdbstr_out.to_debug_string(simplified);
}

}