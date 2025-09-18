#include "serdes/types/parameter_tree.hpp"

#include <nlohmann/json.hpp>

namespace xvorin::serdes {

template <typename T>
class JsonAPI {
public:
    explicit JsonAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    void from_json(const std::string& s);
    void from_json(const std::string& index, const std::string& s);

    std::string to_json(int dump = 4);
    std::string to_json(const std::string& index, int dump = 4);

private:
    ParameterTree<T>& tree_;
};

template <typename T>
void JsonAPI<T>::from_json(const std::string& s)
{
    from_json(tree_.root(), s);
}

template <typename T>
void JsonAPI<T>::from_json(const std::string& index, const std::string& s)
{
    nlohmann::ordered_json jin;
    try {
        jin = nlohmann::ordered_json::parse(s);
    } catch (std::exception& e) {
        throw ParseJsonException(e.what());
    }

    tree_.deserialize(tree_.parameter(index), &jin, ParameterSerdesType::PST_JSON);
    tree_.commit_model_changes();
}

template <typename T>
std::string JsonAPI<T>::to_json(int dump)
{
    return to_json(tree_.root(), dump);
}

template <typename T>
std::string JsonAPI<T>::to_json(const std::string& index, int dump)
{
    tree_.commit_value_changes();

    nlohmann::ordered_json jout;
    tree_.serialize(tree_.parameter(index), &jout, ParameterSerdesType::PST_JSON);

    std::stringstream ss;
    try {
        ss << (dump > 0 ? jout.dump(dump) : jout.dump());
    } catch (std::exception& e) {
        throw DumpJsonException(e.what());
    }

    return ss.str();
}

}