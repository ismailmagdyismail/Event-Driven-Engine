//! Http
#include "HttpRequestParser.h"

//! Helpers
#include "StringHelpers.h"

HttpRequestParser::~HttpRequestParser()
{
    if (m_buffer)
    {
        delete[] m_buffer;
    }
    const unsigned int uiBufferSize = 0;
    Reset(uiBufferSize);
}

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
        return HttpRequestParser::ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Failed,
            .m_strMessage = "Invalid Parsing State Reached",
        };
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
        HttpRequest oRequest = std::move(m_oRequest);
        oRequest.m_bufferData = m_buffer;
        oRequest.m_uiSize = m_uiCurrentRequestSize;
        m_buffer = new char[HttpRequestParser::MAX_REQUEST_SIZE];
        CopyOverNextRequest(m_buffer, m_oRequest.m_bufferData);
        const unsigned int uiNextRequestBufferSize = m_uiCurrentBufferSize - m_uiCurrentRequestSize;
        Reset(uiNextRequestBufferSize);
        return ParsingResult{
            .m_eStatus = ParsingStatus::Done,
            .m_oRequest = std::move(oRequest),
        };
    }

    //! 2. Request size exceeded max set size, and was still not finished.
    if (m_uiCurrentBufferSize >= HttpRequestParser::MAX_REQUEST_SIZE)
    {
        const unsigned int uiBufferSize = 0;
        Reset(uiBufferSize);
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

            //! 2. Split, Parse Request Line
            HttpRequestParser::ParsingResult result = SplitAndStoreRequestLine(m_oRequestLineInfo);
            if (result.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
            {
                return result;
            }

            //! 3. Cache Header Phase Info
            CacheHeaderPhaseInfo(m_oRequestLineInfo);

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
    HttpMethod::Type eHttpMethodType = HttpMethod::Parse(requestLineEntries[0]);
    if (HttpMethod::Type::Unknown == eHttpMethodType)
    {
        return HttpRequestParser::ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Failed,
            .m_strMessage = "Invalid HttpMethod Encountered in RequestLine ",
        };
    }
    m_oRequest.m_sliceRequestLine = slice;
    m_oRequest.m_eMethod = eHttpMethodType;
    m_oRequest.m_sliceURI = requestLineEntries[1];
    //! TODO: Parse Version
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

void HttpRequestParser::CacheHeaderPhaseInfo(const HttpRequestParser::PhaseInfo &p_oRequestLineInfo)
{
    m_oHeaderInfo.m_uiStartPtr = p_oRequestLineInfo.m_uiStartPtr + p_oRequestLineInfo.m_uiSize + 2;
}

HttpRequestParser::ParsingResult HttpRequestParser::ParseHeader()
{
    while (m_uiRequestPtr < m_uiCurrentBufferSize - 3)
    {
        std::string_view slice(m_buffer + m_uiRequestPtr, 4);
        if (slice == "\r\n\r\n")
        {
            //! 1. Create Header Info
            m_oHeaderInfo.m_uiSize = m_uiRequestPtr - m_oHeaderInfo.m_uiStartPtr;

            //! 2. Split and Parse Header
            HttpRequestParser::ParsingResult result = SplitAndStoreHeader(m_oHeaderInfo);
            if (result.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
            {
                return result;
            }

            //! 3. Cache Initial Body Meta-Data Info
            CacheBodyPhaseInfo(m_oHeaderInfo, m_oRequest.m_mapHeaders);

            // 4. Advance ParserState Machine.
            m_uiRequestPtr += 4;
            m_eState = HttpRequestParser::ParserState::Body;
            return ParseBody();
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
        auto value = std::string_view(line.data() + header.size() + 1, line.size() - header.size());
        m_oRequest.m_mapHeaders[header] = value;
    }
    m_oRequest.m_sliceHeader = slice;
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

void HttpRequestParser::CacheBodyPhaseInfo(const HttpRequestParser::PhaseInfo &p_oHeaderPhase, const std::unordered_map<std::string_view, std::string_view> &p_mapHeaders)
{
    //! Cache Body Meta-Data Once
    //! Helps with Parsing Body
    //! Cached , Stored Once after finishing the header parsing
    m_oBodyInfo.m_uiStartPtr = p_oHeaderPhase.m_uiStartPtr + p_oHeaderPhase.m_uiSize + 4;
    auto it = p_mapHeaders.find("Content-Length");
    if (it == p_mapHeaders.end())
    {
        m_oBodyInfo.m_uiSize = 0;
    }
    else
    {
        auto &value = it->second;
        auto strValue = std::string(value.data(), value.size());
        m_oBodyInfo.m_uiSize = stoi(strValue);
    }
}

HttpRequestParser::ParsingResult HttpRequestParser::ParseBody()
{
    if (m_oBodyInfo.m_uiSize == 0)
    {
        m_uiCurrentRequestSize = m_uiRequestPtr;
        m_eState = HttpRequestParser::ParserState::Finished;
        m_oRequest.m_sliceBody = std::string_view{};
        return ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Done,
        };
    }

    unsigned int uiInMemoryBodySize = m_uiCurrentBufferSize - m_uiRequestPtr;
    if (uiInMemoryBodySize < 0)
    {
        throw std::runtime_error("[FATAL]: Error Happend in Body size calculation");
    }

    //! Found whole body in current Consumed Bytes
    //! Thus request is fully finished till end of body
    if (uiInMemoryBodySize >= m_oBodyInfo.m_uiSize)
    {
        m_uiRequestPtr = m_oBodyInfo.m_uiStartPtr + m_oBodyInfo.m_uiSize;
        m_uiCurrentRequestSize = m_uiRequestPtr;
        m_eState = HttpRequestParser::ParserState::Finished;
        m_oRequest.m_sliceBody = std::string_view(m_buffer + m_oBodyInfo.m_uiStartPtr, m_oBodyInfo.m_uiSize);
        return ParsingResult{
            .m_eStatus = HttpRequestParser::ParsingStatus::Done,
        };
    }

    //! Body is still not fully Consumed, present in Memory
    //! Still to be consumed by the input source
    //! So we move consumption ptr till the end of the current size (since all of this belong to the current request)
    //! Body is expected to be in the incoming bytes i.e we are still in progress of parsing body
    m_uiRequestPtr = m_uiCurrentBufferSize;
    return HttpRequestParser::ParsingResult{
        .m_eStatus = HttpRequestParser::ParsingStatus::InProgress,
    };
}

unsigned int HttpRequestParser::GetMaxRemainingRequestSize()
{
    return HttpRequestParser::MAX_REQUEST_SIZE - m_uiCurrentBufferSize;
}

void HttpRequestParser::CopyOverNextRequest(char *currentBuffer, char *oldBuffer)
{
    const unsigned int uiSizeToCopyOver = m_uiCurrentBufferSize - m_uiCurrentRequestSize;
    std::memcpy(currentBuffer, oldBuffer, uiSizeToCopyOver);
}

void HttpRequestParser::Reset(unsigned int p_uiBufferSize)
{
    m_eState = HttpRequestParser::ParserState::RequestLine;
    m_uiCurrentBufferSize = p_uiBufferSize;
    m_uiRequestPtr = 0;
    m_uiCurrentRequestSize = 0;
    ResetPhase(m_oRequestLineInfo);
    ResetPhase(m_oHeaderInfo);
    ResetPhase(m_oBodyInfo);
    m_oRequest = HttpRequest{
        .m_bufferData = m_buffer,
    };
}

void HttpRequestParser::ResetPhase(HttpRequestParser::PhaseInfo &p_oPhase)
{
    p_oPhase.m_uiSize = 0;
    p_oPhase.m_uiStartPtr = 0;
}