#pragma once

//! Http Includes
#include "HttpMethod.h"

//! System Includes
#include <string>
#include <unordered_map>

struct HttpRequest
{
    //! Entire Request
    char *m_bufferData;
    unsigned int m_uiSize;

    //! Request Line
    std::string_view m_sliceRequestLine;
    HttpMethod m_eMethod;
    std::string_view m_sliceURI;
    float version;

    //! Header
    std::string_view m_sliceHeader;
    std::unordered_map<std::string_view, std::string_view> m_mapHeaders;

    //! Body
    std::string_view m_sliceBody;
};
