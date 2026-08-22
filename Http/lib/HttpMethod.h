#pragma once

#include <string>

class HttpMethod
{
public:
    enum class Type
    {
        GET,
        POST,
        DELETE,
        PUT,
        Unknown,
    };

    static HttpMethod::Type Parse(std::string_view &slice);
};