#include "serdes/types/parameter_tree.hpp"

#include <yaml-cpp/yaml.h>

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

    std::string to_yaml();
    std::string to_yaml(const std::string& index);

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
    YAML::Node yin;
    try {
        yin = YAML::Load(s);
    } catch (std::exception& e) {
        throw ParseYamlException(e.what());
    }

    tree_.deserialize(tree_.parameter(index), &yin, ParameterSerdesType::PST_YAML);
    tree_.commit_model_changes();
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

    YAML::Node yout;
    yout.SetStyle(YAML::EmitterStyle::Block);
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