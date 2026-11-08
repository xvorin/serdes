#pragma once

#include <serdes/3rd/pugixml/pugixml.hpp>

#include <cctype>
#include <string>

namespace xvorin::serdes {

// XML 映射规则的公共工具: 供 xml_serdes.hpp(含子树读写)共用
//   元素名 = 参数subkey(可作合法XML标签名时); 否则标签名用param并以name属性记录原始subkey
//   参数comment = XML注释节点(挂接在元素之前, 与toml的"#注释"行风格一致)

// 判断字符串能否直接作为XML元素名: 首字符为字母/下划线/冒号, 其余为字母/数字/下划线/冒号/点/横线
inline bool xml_valid_name(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    const auto head_ok = std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_' || name[0] == ':';
    if (!head_ok) {
        return false;
    }

    for (size_t i = 1; i < name.size(); i++) {
        const auto ok = std::isalnum(static_cast<unsigned char>(name[i])) || name[i] == '_' || name[i] == ':' || name[i] == '.' || name[i] == '-';
        if (!ok) {
            return false;
        }
    }

    return true;
}

// 读取元素对应的subkey: 优先取name属性(非法标签名时的兜底), 否则取标签名
inline std::string xml_subkey_of(const pugi::xml_node& node)
{
    const auto name = node.attribute("name");
    return name ? name.as_string() : node.name();
}

// 挂接子元素: subkey合法时直接作标签名, 否则用固定标签param + name属性记录原始subkey
inline pugi::xml_node xml_append_element(pugi::xml_node& parent, const std::string& subkey)
{
    if (xml_valid_name(subkey)) {
        return parent.append_child(subkey.c_str());
    }

    auto child = parent.append_child("param");
    child.append_attribute("name").set_value(subkey.c_str());
    return child;
}

// 将注释挂接到元素之前(与toml的"#注释"行风格一致); 根元素无父节点时挂接为其子节点
inline void xml_set_comment(pugi::xml_node& node, const std::string& comment)
{
    if (comment.empty()) {
        return;
    }

    auto parent = node.parent();
    if (parent) {
        parent.insert_child_before(pugi::node_comment, node).set_value(comment.c_str());
    } else {
        node.append_child(pugi::node_comment).set_value(comment.c_str());
    }
}

}
