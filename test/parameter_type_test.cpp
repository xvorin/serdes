#include "common.h"

using namespace xvorin::serdes;

TEST(ParameterType, readable_detail_type)
{
    const ParameterDetailType& type = typeid(std::string);
    ASSERT_EQ(Parameter::readable_detail_type(type), "std::string");
}