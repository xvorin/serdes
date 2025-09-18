#pragma once

#include "serdes/serdes/config.h"

namespace xvorin::serdes {

template <typename T>
class MultiSerdes : public Serdes {
public:
    explicit MultiSerdes(const Parameter& owner)
    {
        serdess_ = make_serdess<T>();
        serdess_[ParameterSerdesType::PST_STRU] = std::unique_ptr<StructSerdes<T>>(new StructSerdes<T>());
        serdess_[ParameterSerdesType::PST_STRU]->init(owner);
    }

    void serialize(std::shared_ptr<const Parameter> p, void* out) const override
    {
        auto iter = serdess_.find(p->profile()->serdes_type);
        iter->second->serialize(p, out);
    }

    void deserialize(std::shared_ptr<Parameter> p, const void* in) override
    {
        serdess_[p->profile()->serdes_type]->deserialize(p, in);
    }

private:
    std::map<ParameterSerdesType, std::unique_ptr<Serdes>> serdess_;
};

}