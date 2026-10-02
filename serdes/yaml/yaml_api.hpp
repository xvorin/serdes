#include "serdes/types/parameter_tree.hpp"

#include <serdes/3rd/fkYAML/node.hpp>

namespace fkyaml {
using ordered_node = basic_node<std::vector, fkyaml::ordered_map>;
}

namespace xvorin::serdes {

template <typename T>
class YamlAPI {
public:
    explicit YamlAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    void from_yaml(const std::string& s);
    void from_yaml(const std::string& index, const std::string& s);
    void from_yaml(const std::string& s, T* value);
    void from_yaml(const std::string& index, const std::string& s, T* value);

    std::string to_yaml();
    std::string to_yaml(const std::string& index);
    std::string to_yaml(const T& value);
    std::string to_yaml(const T& value, const std::string& index);

private:
    ParameterTree<T>& tree_;
};

template <typename T>
void YamlAPI<T>::from_yaml(const std::string& s)
{
    from_yaml(tree_.root(), s);
}

template <typename T>
void YamlAPI<T>::from_yaml(const std::string& index, const std::string& s)
{
    from_yaml(index, s, tree_.unsafe_value());
}

template <typename T>
void YamlAPI<T>::from_yaml(const std::string& s, T* value)
{
    from_yaml(tree_.root(), s, value);
}

template <typename T>
void YamlAPI<T>::from_yaml(const std::string& index, const std::string& s, T* value)
{
    fkyaml::ordered_node yin;
    try {
        yin = fkyaml::ordered_node::deserialize(s);
    } catch (std::exception& e) {
        throw ParseYamlException(e.what());
    }

    tree_.deserialize(tree_.parameter(index), &yin, ParameterSerdesType::PST_YAML);
    tree_.commit_model_changes(value);
}

template <typename T>
std::string YamlAPI<T>::to_yaml()
{
    return to_yaml(tree_.root());
}

template <typename T>
std::string YamlAPI<T>::to_yaml(const std::string& index)
{
    tree_.commit_value_changes();

    fkyaml::ordered_node yout;
    tree_.serialize(tree_.parameter(index), &yout, ParameterSerdesType::PST_YAML);

    std::stringstream ss;
    try {
        ss << yout;
    } catch (std::exception& e) {
        throw DumpYamlException(e.what());
    }

    return ss.str();
}

template <typename T>
std::string YamlAPI<T>::to_yaml(const T& value)
{
    return to_yaml(value, tree_.root());
}

template <typename T>
std::string YamlAPI<T>::to_yaml(const T& value, const std::string& index)
{
    tree_.commit_value_changes(value);

    fkyaml::ordered_node yout;
    tree_.serialize(tree_.parameter(index), &yout, ParameterSerdesType::PST_YAML);

    std::stringstream ss;
    try {
        ss << yout;
    } catch (std::exception& e) {
        throw DumpYamlException(e.what());
    }

    return ss.str();
}
}