#pragma once

#include "serdes/types/basic.hpp"
#include "serdes/types/enum.hpp"
#include "serdes/types/map.hpp"
#include "serdes/types/object.hpp"
#include "serdes/types/sequence.hpp"
#include "serdes/types/set.hpp"

#include <mutex>

namespace xvorin::serdes {

class Serdes {
public:
    virtual ~Serdes() = default;

    virtual void init(const Parameter& owner) { }

    virtual void serialize(std::shared_ptr<const Parameter>, void* out) const = 0;
    virtual void deserialize(std::shared_ptr<Parameter>, const void* in) = 0;
};

}