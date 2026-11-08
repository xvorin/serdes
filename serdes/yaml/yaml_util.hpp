#pragma once

#include <serdes/3rd/fkYAML/node.hpp>

namespace fkyaml {

// 保序的 YAML DOM 别名: 供 yaml_serdes.hpp(含子树读写)共用
using ordered_node = basic_node<std::vector, fkyaml::ordered_map>;

}
