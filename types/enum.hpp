#pragma once

#include "serdes/types/parameter.hpp"
#include "serdes/types/traits.hpp"

namespace xvorin::serdes {

template <typename T>
class MultiSerdes;

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_enum<T>::value>::type> : public Parameter {
public:
    using subtype = T;
    using inform_type = std::function<bool(const std::string&, T, T)>;

public:
    TraitedParameter(std::string subkey, size_t offset, std::string comment, inform_type inform, std::string verinfo);

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override;

    virtual void serialize(void* out) const override;
    virtual void deserialize(const void* in) override;

    virtual std::string debug_self() const override;
    virtual std::string debug_releation(const std::string& prefix = "") const override;

    virtual void inform_ancestor(const std::string& index, ParameterInformType pit) const override;

public:
    T value;
    const inform_type inform;

private:
    std::shared_ptr<MultiSerdes<T>> serdes_;
};

template <typename T>
using EnumParameter = TraitedParameter<T, typename std::enable_if<is_enum<T>::value>::type>;

}