#include "StringHelpers.h"

#include <vector>

std::vector<std::string_view> Split(std::string_view str, std::string_view delimiter)
{
    std::vector<std::string_view> result;

    size_t start = 0;

    while (start <= str.size())
    {
        size_t pos = str.find(delimiter, start);

        if (pos == std::string_view::npos)
        {
            result.emplace_back(str.substr(start));
            break;
        }

        result.emplace_back(str.substr(start, pos - start));
        start = pos + delimiter.size();
    }

    return result;
}