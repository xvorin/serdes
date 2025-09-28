#pragma once

#include "serdes/types/parameter.hpp"
#include <unordered_map>

namespace xvorin::serdes {

struct ParameterPrototype {
public:
    static std::shared_ptr<Parameter> create_parameter(const ParameterDetailType& type, const std::string& newkey, size_t offset = 0)
    {
        auto proto = ParameterPrototype::query_prototype(type);
        if (!proto) {
            throw PrototypeNotFound(Parameter::readable_detail_type(type) + " for " + newkey);
        }

        std::shared_ptr<Parameter> created = proto->clone(newkey, offset);

        if (created->type != ParameterType::PT_OBJECT) {
            return created;
        }

        auto children = created->mutable_children();

        for (auto& child_pair : (*children)) {
            auto& ochild = child_pair.second;
            if (ochild->type != ParameterType::PT_OBJECT) {
                continue;
            }

            auto nchild = ParameterPrototype::create_parameter(ochild->detail, ochild->subkey, ochild->offset);
            const_cast<std::string&>(nchild->comment) = (nchild->comment.size() > ochild->comment.size()) ? nchild->comment : ochild->comment;
            const_cast<std::string&>(nchild->verinfo) = (nchild->verinfo.size() > ochild->verinfo.size()) ? nchild->verinfo : ochild->verinfo;
            nchild->parent = created;

            ochild.swap(nchild);
        }

        return created;
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

    static std::shared_ptr<Parameter> query_prototype(const ParameterDetailType& type)
    {
        const auto& prototypes = container();
        auto iter = prototypes.find(type);
        return (iter == prototypes.end()) ? nullptr : iter->second;
    }

    static void insert_prototype(const ParameterDetailType& type, std::shared_ptr<Parameter> proto)
    {
        auto& prototypes = container();
        auto iter = prototypes.find(type);

        if (iter != prototypes.end()) {
            throw PrototypeDuplicate(Parameter::readable_detail_type(type));
        }
        prototypes[type] = proto;
    }

    friend class ParameterRegistrar;
};
}