#pragma once

//! Http Includes
#include "HttpMethod.h"

//! System Includes
#include <string>
#include <unordered_map>

struct HttpRequest
{
    void Destroy()
    {
        if (m_bufferData)
        {
            delete[] m_bufferData;
        }
    }

    //! Entire Request
    char *m_bufferData{nullptr};
    unsigned int m_uiSize{0};

    //! Request Line
    std::string_view m_sliceRequestLine;
    HttpMethod::Type m_eMethod;
    std::string_view m_sliceURI;
    float version;

    //! Header
    std::string_view m_sliceHeader;
    std::unordered_map<std::string_view, std::string_view> m_mapHeaders;

    //! Body
    std::string_view m_sliceBody;
};
