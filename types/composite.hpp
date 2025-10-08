#pragma once

#include "serdes/types/parameter.hpp"
#include "serdes/types/prototype.hpp"

namespace xvorin::serdes {

template <typename T>
class MultiSerdes;

template <typename T>
struct Composite : public Parameter {
public:
    using SortCompareCallback = std::function<bool(std::shared_ptr<Parameter> a, std::shared_ptr<Parameter> b)>;
    using informant = std::function<void(const std::string&, ParameterInformType pit)>;

public:
    Composite(std::string subkey, ParameterDetailType detail, size_t offset, std::string comment, informant inform, std::string verinfo);

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override;

    virtual void serialize(void* out) const override;
    virtual void deserialize(const void* in) override;

    virtual std::shared_ptr<Parameter> find_child(const std::string& subkey) const override;
    virtual const std::unordered_map<std::string, std::shared_ptr<Parameter>>& children() const override;
    virtual std::unordered_map<std::string, std::shared_ptr<Parameter>>* mutable_children() override;

    virtual void inform_ancestor(const std::string& index, ParameterInformType pit) const override;

    virtual std::string debug_self() const override;
    virtual std::string debug_releation(const std::string& prefix = "") const override;

    void insert_child(std::shared_ptr<Parameter> param);
    std::list<std::shared_ptr<Parameter>> sorted_children(SortCompareCallback cb = nullptr) const;

public:
    const informant inform;

protected:
    SortCompareCallback sort_children_strategy;

private:
    std::shared_ptr<MultiSerdes<T>> serdes_;
    std::unordered_map<std::string, std::shared_ptr<Parameter>> children_;
};

}
