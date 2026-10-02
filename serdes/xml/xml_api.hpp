#include "serdes/serdes/config.h"
#include "serdes/types/parameter_tree.hpp"

#include <serdes/3rd/pugixml/pugixml.hpp>

#include <sstream>

namespace xvorin::serdes {

template <typename T>
class XmlAPI {
public:
    explicit XmlAPI(ParameterTree<T>& tree)
        : tree_(tree)
    {
    }

    void from_xml(const std::string& s);
    void from_xml(const std::string& index, const std::string& s);
    void from_xml(const std::string& s, T* value);
    void from_xml(const std::string& index, const std::string& s, T* value);

    std::string to_xml(int indent = 4);
    std::string to_xml(const std::string& index, int indent = 4);
    std::string to_xml(const T& value, int indent = 4);
    std::string to_xml(const T& value, const std::string& index, int indent = 4);

private:
    ParameterTree<T>& tree_;
};

template <typename T>
void XmlAPI<T>::from_xml(const std::string& s)
{
    from_xml(tree_.root(), s);
}

template <typename T>
void XmlAPI<T>::from_xml(const std::string& index, const std::string& s)
{
    from_xml(index, s, tree_.unsafe_value());
}

template <typename T>
void XmlAPI<T>::from_xml(const std::string& s, T* value)
{
    from_xml(tree_.root(), s, value);
}

template <typename T>
void XmlAPI<T>::from_xml(const std::string& index, const std::string& s, T* value)
{
    pugi::xml_document xin;
    pugi::xml_parse_result result = xin.load_string(s.c_str());
    if (!result) {
        throw ParseXmlException(result.description());
    }

    // XML要求唯一的根元素, 根元素名不参与索引定位
    pugi::xml_node root = xin.document_element();
    if (!root) {
        throw ParseXmlException("no root element");
    }

    tree_.deserialize(tree_.parameter(index), &root, ParameterSerdesType::PST_XML);
    tree_.commit_model_changes(value);
}

template <typename T>
std::string XmlAPI<T>::to_xml(int indent)
{
    return to_xml(tree_.root(), indent);
}

template <typename T>
std::string XmlAPI<T>::to_xml(const std::string& index, int indent)
{
    tree_.commit_value_changes();

    auto parameter = tree_.parameter(index);

    pugi::xml_document xout;
    pugi::xml_node root = xml_append_element(xout, parameter->subkey);
    tree_.serialize(parameter, &root, ParameterSerdesType::PST_XML);

    std::stringstream ss;
    try {
        const auto flags = indent > 0 ? pugi::format_indent : pugi::format_raw;
        const auto indent_str = indent > 0 ? std::string(static_cast<size_t>(indent), ' ') : std::string();
        xout.save(ss, indent_str.c_str(), flags);
    } catch (std::exception& e) {
        throw DumpXmlException(e.what());
    }

    return ss.str();
}

template <typename T>
std::string XmlAPI<T>::to_xml(const T& value, int indent)
{
    return to_xml(value, tree_.root(), indent);
}

template <typename T>
std::string XmlAPI<T>::to_xml(const T& value, const std::string& index, int indent)
{
    tree_.commit_value_changes(value);

    auto parameter = tree_.parameter(index);

    pugi::xml_document xout;
    pugi::xml_node root = xml_append_element(xout, parameter->subkey);
    tree_.serialize(parameter, &root, ParameterSerdesType::PST_XML);

    std::stringstream ss;
    try {
        const auto flags = indent > 0 ? pugi::format_indent : pugi::format_raw;
        const auto indent_str = indent > 0 ? std::string(static_cast<size_t>(indent), ' ') : std::string();
        xout.save(ss, indent_str.c_str(), flags);
    } catch (std::exception& e) {
        throw DumpXmlException(e.what());
    }

    return ss.str();
}
}
