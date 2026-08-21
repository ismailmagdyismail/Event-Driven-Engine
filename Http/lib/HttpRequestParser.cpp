#include "HttpRequestParser.h"

HttpRequestParser::ParsingResult HttpRequestParser::Parse()
{
    HttpRequestParser::ParsingResult result;
    switch (static_cast<unsigned int>(m_eState))
    {
    case static_cast<unsigned int>(HttpRequestParser::ParserState::RequestLine):
        result = ParseRequestLine();
        break;
    case static_cast<unsigned int>(HttpRequestParser::ParserState::Header):
        result = ParseHeader();
        break;
    case static_cast<unsigned int>(HttpRequestParser::ParserState::Body):
        result = ParseBody();
        break;
    default:
        break;
    }

    if (result.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
    {
        return result;
    }

    //! 1. Request is finished
    //! 1.1 Finished and no other requests following it
    //! 1.2 Finished and other requests follow it
    if (m_eState == HttpRequestParser::ParserState::Finished)
    {
        return ParsingResult{
            .m_eStatus = ParsingStatus::Done,
        };
    }

    //! 2. Request size exceeded max set size, and was still not finished.
    if (m_uiCurrentBufferSize >= HttpRequestParser::MAX_REQUEST_SIZE)
    {
        return ParsingResult{
            .m_eStatus = ParsingStatus::Failed,
            .m_strMessage = "Http Request exceeded max size, without being finished",
        };
    };

    //! 3. Still in progress, Parsing huge Payload for example
    return ParsingResult{
        .m_eStatus = ParsingStatus::InProgress,
    };
}

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

HttpRequestParser::ParsingResult HttpRequestParser::ParseRequestLine()
{
    while (m_uiRequestPtr < m_uiCurrentBufferSize - 1)
    {
        std::string_view slice(m_buffer + m_uiRequestPtr, 2);
        if (slice == "\r\n")
        {
            //! 1. Create Request Line info, And Beggining of Header info
            m_oRequestLineInfo.m_uiStartPtr = 0;
            m_oRequestLineInfo.m_uiSize = m_uiRequestPtr;
            m_oHeaderInfo.m_uiStartPtr = m_oRequestLineInfo.m_uiStartPtr + m_oRequestLineInfo.m_uiSize + 2;

            //! 2. Split, Parse Request Line
            HttpRequestParser::ParsingResult result = SplitAndStoreRequestLine(m_oRequestLineInfo);
            if (result.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
            {
                return result;
            }

            //! 3. Advance state machine, and Parse next phase
            m_uiRequestPtr += 2;
            m_eState = HttpRequestParser::ParserState::Header;
            return ParseHeader();
        }
        else
        {
            ++m_uiRequestPtr;
        }
    }
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

HttpRequestParser::ParsingResult HttpRequestParser::SplitAndStoreRequestLine(const HttpRequestParser::PhaseInfo &p_oRequestLineInfo)
{
    std::string_view slice(m_buffer + p_oRequestLineInfo.m_uiStartPtr, p_oRequestLineInfo.m_uiSize);
    auto requestLineEntries = Split(slice, " ");
    if (requestLineEntries.size() != 3)
    {
        return HttpRequestParser::ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Failed,
            .m_strMessage = "RequestLine doesn't has " + std::to_string(requestLineEntries.size()) + " Instead of 3",
        };
    }
    m_oRequest.m_sliceURI = requestLineEntries[1];
    //! TODO: Parse HttpMethod into enum
    //! TODO: Parse Version
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

HttpRequestParser::ParsingResult HttpRequestParser::ParseHeader()
{
    while (m_uiRequestPtr < m_uiCurrentBufferSize - 3)
    {
        std::string_view slice(m_buffer + m_uiRequestPtr, 4);
        if (slice == "\r\n\r\n")
        {
            //! 1. Create Header Info, And Body Info
            m_oHeaderInfo.m_uiSize = m_uiRequestPtr - m_oHeaderInfo.m_uiStartPtr;
            m_oBodyInfo.m_uiStartPtr = m_oHeaderInfo.m_uiStartPtr + m_oHeaderInfo.m_uiSize + 4;

            //! 2. Split and Parse Header
            HttpRequestParser::ParsingResult result = SplitAndStoreHeader(m_oHeaderInfo);
            if (result.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
            {
                return result;
            }

            // 3. Advance ParserState Machine.
            m_uiRequestPtr += 4;
            m_eState = HttpRequestParser::ParserState::Body;
            ParseBody();
        }
        else
        {
            ++m_uiRequestPtr;
        }
    }
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

HttpRequestParser::ParsingResult HttpRequestParser::SplitAndStoreHeader(const HttpRequestParser::PhaseInfo &p_oHeaderPhaseInfo)
{
    std::string_view slice(m_buffer + p_oHeaderPhaseInfo.m_uiStartPtr, p_oHeaderPhaseInfo.m_uiSize);
    auto lines = Split(slice, "\r\n");
    for (auto &line : lines)
    {
        auto header_value = Split(line, ":");
        if (header_value.size() < 2)
        {
            return HttpRequestParser::ParsingResult{
                .m_eStatus = HttpRequestParser::ParsingStatus::Failed,
                .m_strMessage = "Invalid Http Header Found " + std::string(line.data(), line.size()),
            };
        }
        auto header = header_value[0];
        auto value = std::string_view(line.data() + header.size(), line.size() - header.size());
        m_oRequest.m_mapHeaders[header] = value;
    }
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

HttpRequestParser::ParsingResult HttpRequestParser::ParseBody()
{
    auto it = m_oRequest.m_mapHeaders.find("ContentLength");
    if (it == m_oRequest.m_mapHeaders.end())
    {
        m_uiCurrentRequestSize = m_uiRequestPtr;
        m_eState = HttpRequestParser::ParserState::Finished;
        return ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Done,
        };
    }

    auto value = m_oRequest.m_mapHeaders["ContentLength"];
    auto strValue = std::string(value.data(), value.size());

    //! TODO: cache this info after parsing header
    int uiBodySize = stoi(strValue);
    m_oBodyInfo.m_uiSize = uiBodySize;
    int uiInMemoryBodySize = m_uiCurrentBufferSize - m_uiRequestPtr;
    if (uiInMemoryBodySize < 0)
    {
        throw std::runtime_error("[FATAL]: Error Happend in Body size calculation");
    }
    if (uiInMemoryBodySize > uiBodySize)
    {
        m_uiRequestPtr = m_oBodyInfo.m_uiStartPtr + uiBodySize;
        m_uiCurrentRequestSize = m_uiRequestPtr;
        m_eState = HttpRequestParser::ParserState::Finished;
        return ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Done,
        };
    }
    m_uiRequestPtr = m_uiCurrentBufferSize;
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

void HttpRequestParser::StoreBody(HttpRequestParser::PhaseInfo &p_oBodyInfo)
{
    m_oRequest.m_sliceBody = std::string_view(m_buffer + p_oBodyInfo.m_uiStartPtr, p_oBodyInfo.m_uiSize);
}

unsigned int HttpRequestParser::GetMaxRemainingRequestSize()
{
    return HttpRequestParser::MAX_REQUEST_SIZE - m_uiCurrentBufferSize;
}