#include "HttpRequestParser.h"

template <typename InputSource>
int HttpRequestParser::ReadIntoBuffer(InputSource &tSource)
{
    return tSource.ReadSync(m_buffer + m_uiCurrentBufferSize, GetMaxRemainingRequestSize());
}

template <typename InputSource>
HttpRequestParser::ParsingResult HttpRequestParser::Parse(InputSource &tSource)
{
    int uiReadBytesCount = ReadIntoBuffer(tSource);
    m_uiCurrentBufferSize += uiReadBytesCount;
    return Parse();
}