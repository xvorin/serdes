#include "serdes/types/parameter_tree.hpp"

#include <toml11/toml.hpp>

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

    std::string to_toml();
    std::string to_toml(const std::string& index);

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
    toml::value tin;
    try {
        tin = toml::parse_str(s);
    } catch (std::exception& e) {
        throw ParseTomlException(e.what());
    }

    tree_.deserialize(tree_.parameter(index), &tin, ParameterSerdesType::PST_TOML);
    tree_.commit_model_changes();
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

    toml::value tout;
    tree_.serialize(tree_.parameter(index), &tout, ParameterSerdesType::PST_TOML);

    std::stringstream ss;
    try {
        ss << toml::format(tout);
    } catch (std::exception& e) {
        throw DumpTomlException(e.what());
    }

    return ss.str();
}

}