#pragma once

#include "serdes/types/parameter.hpp"
#include <unordered_map>

namespace xvorin::serdes {

struct ParameterPrototype {
public:
    template <typename T = Parameter>
    static std::shared_ptr<T> create_parameter(const ParameterDetailType& type, const std::string& newkey, size_t offset = 0)
    {
        auto temp = ParameterPrototype::prototype<T>(type);
        if (!temp) {
            throw PrototypeNotFound(Parameter::readable_detail_type(type) + " for " + newkey);
        }

        return std::static_pointer_cast<T>(temp->clone(newkey, offset));
    }

    static std::string debug_string()
    {
        bool first_item = true;
        std::stringstream ss;
        for (auto& temp : container()) {
            ss << (first_item ? (first_item = !first_item, "") : "\n");
            ss << "PROTOTYPE:"
               << Parameter::readable_detail_type(temp.first)
               << std::endl
               << temp.second->debug_releation("    ");
        }
        return ss.str();
    }

private:
    using PrototypeContainer = std::unordered_map<ParameterDetailType, std::shared_ptr<Parameter>,
        ParameterDetailTypeHash, ParameterDetailTypeEqual>;

    static PrototypeContainer& container()
    {
        static PrototypeContainer prototypes;
        return prototypes;
    }

    template <typename T = Parameter>
    static std::shared_ptr<T> prototype(const ParameterDetailType& type)
    {
        const auto& prototypes = container();
        auto iter = prototypes.find(type);
        return iter == prototypes.end() ? nullptr : std::dynamic_pointer_cast<T>(iter->second);
    }

    static void insert_prototype(const ParameterDetailType& type, std::shared_ptr<Parameter> templ)
    {
        auto& prototypes = container();
        auto iter = prototypes.find(type);

        if (iter != prototypes.end()) {
            throw PrototypeDuplicate(Parameter::readable_detail_type(type));
        }
        prototypes[type] = templ;
    }

    friend class ParameterRegistrar;
};

}