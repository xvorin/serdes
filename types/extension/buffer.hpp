#pragma once

#include <string>

namespace xvorin::serdes {

class buffer : public std::string {
public:
    using std::string::string;
    buffer() = default;
    buffer(const std::string& s)
        : std::string(s)
    {
    }
};

}