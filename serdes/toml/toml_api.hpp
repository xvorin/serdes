#include "serdes/types/parameter_tree.hpp"

#include <serdes/3rd/toml11/toml.hpp>

namespace xvorin::serdes {

template <typename T>
class TomlAPI {
public:
    explicit TomlAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    void from_toml(const std::string& s);
    void from_toml(const std::string& index, const std::string& s);
    void from_toml(const std::string& s, T* value);
    void from_toml(const std::string& index, const std::string& s, T* value);

    std::string to_toml();
    std::string to_toml(const std::string& index);
    std::string to_toml(const T& value);
    std::string to_toml(const T& value, const std::string& index);

private:
    ParameterTree<T>& tree_;
};

template <typename T>
void TomlAPI<T>::from_toml(const std::string& s)
{
    from_toml(tree_.root(), s);
}

template <typename T>
void TomlAPI<T>::from_toml(const std::string& index, const std::string& s)
{
    from_toml(index, s, tree_.unsafe_value());
}

template <typename T>
void TomlAPI<T>::from_toml(const std::string& s, T* value)
{
    from_toml(tree_.root(), s, value);
}

template <typename T>
void TomlAPI<T>::from_toml(const std::string& index, const std::string& s, T* value)
{
    toml::ordered_value tin;
    try {
        toml::spec spec = toml::spec::v(1, 0, 0);
        spec.ext_null_value = true; // this allows `key = null` value
        tin = toml::parse_str(s, spec);
    } catch (std::exception& e) {
        throw ParseTomlException(e.what());
    }

    tree_.deserialize(tree_.parameter(index), &tin, ParameterSerdesType::PST_TOML);
    tree_.commit_model_changes(value);
}

template <typename T>
std::string TomlAPI<T>::to_toml()
{
    return to_toml(tree_.root());
}

template <typename T>
std::string TomlAPI<T>::to_toml(const std::string& index)
{
    tree_.commit_value_changes();

    toml::ordered_value tout;
    tree_.serialize(tree_.parameter(index), &tout, ParameterSerdesType::PST_TOML);

    std::stringstream ss;
    try {
        toml::spec spec = toml::spec::v(1, 0, 0);
        spec.ext_null_value = true; // this allows `key = null` value
        ss << toml::format(tout, spec);
    } catch (std::exception& e) {
        throw DumpTomlException(e.what());
    }

    return ss.str();
}

template <typename T>
std::string TomlAPI<T>::to_toml(const T& value)
{
    return to_toml(value, tree_.root());
}

template <typename T>
std::string TomlAPI<T>::to_toml(const T& value, const std::string& index)
{
    tree_.commit_value_changes(value);

    toml::ordered_value tout;
    tree_.serialize(tree_.parameter(index), &tout, ParameterSerdesType::PST_TOML);

    std::stringstream ss;
    try {
        toml::spec spec = toml::spec::v(1, 0, 0);
        spec.ext_null_value = true; // this allows `key = null` value
        ss << toml::format(tout, spec);
    } catch (std::exception& e) {
        throw DumpTomlException(e.what());
    }

    return ss.str();
}
}