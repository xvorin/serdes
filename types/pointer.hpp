#pragma once

#include "serdes/types/parameter.hpp"
#include "serdes/types/traits.hpp"

namespace xvorin::serdes {

template <typename T>
struct TraitedParameter<T, typename std::enable_if<is_smart_ptr<T>::value>::type> : public Parameter {
    using subtype = typename T::element_type;
    using informant = std::function<void(const std::string&, ParameterInformType pit)>;

    TraitedParameter(std::string subkey, size_t offset,
        std::string comment, informant inform, std::string verinfo)
        : Parameter(std::move(subkey), ParameterType::PT_PTR, typeid(subtype), offset, std::move(comment), std::move(verinfo))
        , inform(std::move(inform))
    {
        serdes_ = std::make_shared<MultiSerdes<T>>(*this);
    }

    virtual std::shared_ptr<Parameter> clone(std::string newkey, size_t newoffset) const override
    {
        auto cloned = std::make_shared<TraitedParameter<T>>(std::move(newkey), newoffset, comment, inform, verinfo);
        cloned->parent = parent;
        if (value) {
            cloned->value = value->clone(value->subkey, value->offset);
            cloned->value->parent = cloned;
        }
        return cloned;
    }

    virtual void serialize(void* out) const override
    {
        serdes_->serialize(shared_from_this(), out);
    }

    virtual void deserialize(const void* in) override
    {
        serdes_->deserialize(shared_from_this(), in);
    }

    virtual void create_child(const std::string& newkey) override
    {
        if (value) {
            throw IndexDuplicate(this->index() + "." + newkey);
        }

        value = ParameterPrototype::create_parameter(this->detail, "0");
        value->parent = this->shared_from_this();
    }

    virtual void remove_child(const std::string& subkey) override
    {
        if (!value || subkey != "0") {
            throw ParameterNotFound(this->index() + "." + subkey);
        }
        value = nullptr;
    }

    virtual std::shared_ptr<Parameter> find_child(const std::string& subkey) const override
    {
        return value;
    }

    T make_smart_ptr() const
    {
        return T(new subtype());
    }

    virtual std::string debug_self() const override
    {
        std::stringstream ss;
        ss << (value ? "" : " = nullptr") << " [" + this->readable_detail_type() + "]";
        if (!comment.empty()) {
            ss << " #" << comment;
        }
        return ss.str();
    }

    virtual std::string debug_releation(const std::string& prefix = "") const override
    {
        std::stringstream ss;
        ss << prefix << "|" << index() << debug_self();
        if (value) {
            ss << std::endl
               << value->debug_releation(prefix + "| ");
        }
        return ss.str();
    }

public:
    std::shared_ptr<Parameter> value;
    informant inform;

private:
    std::shared_ptr<MultiSerdes<T>> serdes_;
};

}