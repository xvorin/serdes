#pragma once

#include <algorithm>
#include <exception>
#include <list>
#include <sstream>
#include <unordered_map>

namespace xvorin::serdes {

// error messages
namespace error_msg {

const char* const INVALID_INDEX = "invalid parameter index";
const char* const PARAMETER_NOT_FOUND = "parameter not found";
const char* const BAD_FILE = "bad file";

const char* const PARSE_JSON_ERROR = "parse json error";
const char* const PARSE_YAML_ERROR = "parse yaml error";
const char* const PARSE_TOML_ERROR = "parse toml error";

const char* const DUMP_JSON_ERROR = "dump json error";
const char* const DUMP_YAML_ERROR = "dump yaml error";
const char* const DUMP_TOML_ERROR = "dump toml error";

const char* const VALUE_CHECK_ERROR = "value check error";
const char* const TYPE_NOT_SUPPORT = "type not support";
const char* const INDEX_DUMPLICATE = "index duplicate while create";

const char* const NEED_ROOT_PROFILE = "Need ParameterProfile for Root Parameter";

const char* const PROTOTYPE_NOT_FOUND = "Prototype not found";
const char* const PROTOTYPE_DUMPLICATE = "Prototype duplicate";

const char* const SINKFILE_NO_SPECIFIED = "sink file no specified";

const char* const PROFILE_ONLY_ROOT = "profile can only be set in root";

const char* const ENVAR_NOT_EXIST = "environment variable does not exist";

}

class Exception : public std::runtime_error {
public:
    Exception(const std::string& msg, const std::string& extension = "")
        : std::runtime_error(extension.empty() ? msg : (msg + ":" + extension))
    {
    }
    virtual ~Exception() = default;
    Exception(const Exception&) = default;
};

class InvalidIndex : public Exception {
public:
    InvalidIndex(const std::string& index)
        : Exception(error_msg::INVALID_INDEX, index)
    {
    }
};

class IndexDuplicate : public Exception {
public:
    IndexDuplicate(const std::string& index)
        : Exception(error_msg::INDEX_DUMPLICATE, index)
    {
    }
};

class ParameterNotFound : public Exception {
public:
    ParameterNotFound(const std::string& index)
        : Exception(error_msg::PARAMETER_NOT_FOUND, index)
    {
    }

    ParameterNotFound(const std::list<std::string>& index)
        : Exception(error_msg::PARAMETER_NOT_FOUND, reduce_index(index))
    {
    }

    std::string reduce_index(const std::list<std::string>& index)
    {
        std::stringstream ss;
        std::for_each(index.begin(), --(index.end()), [&](const std::string& s) { ss << s << "."; });
        ss << index.back();
        return ss.str();
    }
};

class BadFile : public Exception {
public:
    BadFile(const std::string& file)
        : Exception(error_msg::BAD_FILE, file)
    {
    }
};

class ParseJsonException : public Exception {
public:
    ParseJsonException(const std::string& extension)
        : Exception(error_msg::PARSE_JSON_ERROR, extension)
    {
    }
};

class ParseYamlException : public Exception {
public:
    ParseYamlException(const std::string& extension)
        : Exception(error_msg::PARSE_YAML_ERROR, extension)
    {
    }
};

class ParseTomlException : public Exception {
public:
    ParseTomlException(const std::string& extension)
        : Exception(error_msg::PARSE_TOML_ERROR, extension)
    {
    }
};

class DumpJsonException : public Exception {
public:
    DumpJsonException(const std::string& extension)
        : Exception(error_msg::DUMP_JSON_ERROR, extension)
    {
    }
};

class DumpYamlException : public Exception {
public:
    DumpYamlException(const std::string& extension)
        : Exception(error_msg::DUMP_YAML_ERROR, extension)
    {
    }
};

class DumpTomlException : public Exception {
public:
    DumpTomlException(const std::string& extension)
        : Exception(error_msg::DUMP_TOML_ERROR, extension)
    {
    }
};

class ViolationException : public Exception {
public:
    ViolationException(const std::unordered_map<std::string, std::pair<std::string, std::string>>& extension)
        : Exception(error_msg::VALUE_CHECK_ERROR, to_string(extension))
    {
    }

private:
    std::string to_string(const std::unordered_map<std::string, std::pair<std::string, std::string>>& extension)
    {
        std::string retval;
        for (auto ex : extension) {
            retval.append("\n")
                .append(ex.first)
                .append(" != ")
                .append(ex.second.first)
                .append(" ")
                .append(ex.second.second);
        }
        return retval;
    }
};

class TypeNotSupport : public Exception {
public:
    TypeNotSupport(const std::string& extension)
        : Exception(error_msg::TYPE_NOT_SUPPORT, extension)
    {
    }
};

class NeedRootProfile : public Exception {
public:
    NeedRootProfile()
        : Exception(error_msg::NEED_ROOT_PROFILE)
    {
    }
};

class PrototypeNotFound : public Exception {
public:
    PrototypeNotFound(const std::string& extension)
        : Exception(error_msg::PROTOTYPE_NOT_FOUND, extension)
    {
    }
};

class PrototypeDuplicate : public Exception {
public:
    PrototypeDuplicate(const std::string& extension)
        : Exception(error_msg::PROTOTYPE_DUMPLICATE, extension)
    {
    }
};

class SinkfileNoSpecified : public Exception {
public:
    SinkfileNoSpecified(const std::string& extension)
        : Exception(error_msg::SINKFILE_NO_SPECIFIED, extension)
    {
    }
};

class ProfileOnlyRoot : public Exception {
public:
    ProfileOnlyRoot(const std::string& extension)
        : Exception(error_msg::PROFILE_ONLY_ROOT, extension)
    {
    }
};

class EnvarNotExist : public Exception {
public:
    EnvarNotExist(const std::string& extension)
        : Exception(error_msg::ENVAR_NOT_EXIST, extension)
    {
    }
};

}
