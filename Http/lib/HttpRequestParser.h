#pragma once

//! Http Includes
#include "HttpRequest.h"

//! System Includes
#include <string>

class HttpRequestParser
{
public:
    static const unsigned int MAX_REQUEST_SIZE = 1024 * 1024 * 10;

    enum class ParsingStatus
    {
        InProgress,
        Done,
        Failed,
    };

    struct ParsingResult
    {
        ParsingStatus m_eStatus;
        std::string m_strMessage;
    };

    template <typename InputSource>
    HttpRequestParser::ParsingResult Parse(InputSource &tSource);

private:
    template <typename InputSource>
    int ReadIntoBuffer(InputSource &tSource);
    HttpRequestParser::ParsingResult Parse();

    struct PhaseInfo
    {
        unsigned int m_uiStartPtr;
        unsigned int m_uiSize; // NOT Including \r\n
    };

    enum class ParserState
    {
        RequestLine,
        Header,
        Body,
        Finished,
    };

    //! Request Line Parsing
    HttpRequestParser::ParsingResult ParseRequestLine();
    HttpRequestParser::ParsingResult SplitAndStoreRequestLine(const HttpRequestParser::PhaseInfo &p_oRequestLineInfo);

    //! Header Parsing
    HttpRequestParser::ParsingResult ParseHeader();
    HttpRequestParser::ParsingResult SplitAndStoreHeader(const HttpRequestParser::PhaseInfo &p_oHeaderPhaseInfo);

    //! Body Parsing
    HttpRequestParser::ParsingResult ParseBody();
    void StoreBody(HttpRequestParser::PhaseInfo &p_oBodyInfo);

    //! Sizing Calculations
    unsigned int GetMaxRemainingRequestSize();

    HttpRequestParser::ParserState m_eState{HttpRequestParser::ParserState::RequestLine};
    unsigned int m_uiCurrentBufferSize{0};
    unsigned int m_uiCurrentRequestSize{0};
    unsigned int m_uiRequestPtr{0};
    char m_buffer[HttpRequestParser::MAX_REQUEST_SIZE];
    HttpRequestParser::PhaseInfo m_oRequestLineInfo;
    HttpRequestParser::PhaseInfo m_oHeaderInfo;
    HttpRequestParser::PhaseInfo m_oBodyInfo;
    HttpRequest m_oRequest;
};

#include "HttpRequestParser.ipp"