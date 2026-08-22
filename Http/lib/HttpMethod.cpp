//! System Includes
#include <string>
#include <unordered_map>

//! Http
#include "HttpMethod.h"

static std::unordered_map<std::string_view, HttpMethod::Type> mapHttpMethods{
    {"GET", HttpMethod::Type::GET},
    {"POST", HttpMethod::Type::POST},
    {"PUT", HttpMethod::Type::PUT},
    {"DELETE", HttpMethod::Type::DELETE},
};

HttpMethod::Type HttpMethod::Parse(std::string_view &slice)
{
    auto it = mapHttpMethods.find(slice);
    if (it == mapHttpMethods.end())
    {
        return HttpMethod::Type::Unknown;
    }
    return it->second;
}