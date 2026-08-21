#include <iostream>
#include "EventLoop.h"
#include "TCPServerSocket.h"

#include <string_view>
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

enum class HttpMethod
{
    GET,
    POST,
    DELETE,
    PUT,
};

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

class HttpRequestParser
{
public:
    static const unsigned int MAX_REQUEST_SIZE = 1024 * 1024 * 10;

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
    HttpRequestParser::ParsingResult Parse(InputSource &tSource)
    {
        unsigned int uiReadBytesCount = tSource.ReadSync(m_buffer + m_uiCurrentBufferSize, GetMaxRemainingRequestSize());
        m_uiCurrentBufferSize += uiReadBytesCount;
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

private:
    HttpRequestParser::ParsingResult ParseRequestLine()
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

    HttpRequestParser::ParsingResult SplitAndStoreRequestLine(const HttpRequestParser::PhaseInfo &p_oRequestLineInfo)
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

    HttpRequestParser::ParsingResult ParseHeader()
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

    HttpRequestParser::ParsingResult SplitAndStoreHeader(const HttpRequestParser::PhaseInfo &p_oHeaderPhaseInfo)
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

    HttpRequestParser::ParsingResult ParseBody()
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

    void StoreBody(HttpRequestParser::PhaseInfo &p_oBodyInfo)
    {
        m_oRequest.m_sliceBody = std::string_view(m_buffer + p_oBodyInfo.m_uiStartPtr, p_oBodyInfo.m_uiSize);
    }

    unsigned int GetMaxRemainingRequestSize()
    {
        return HttpRequestParser::MAX_REQUEST_SIZE - m_uiCurrentBufferSize;
    }

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

// struct RequestContext
// {
//     AsyncIO::TCPClientSocket *m_pClientConnection;
//     unsigned long long m_ullConnectionID;
//     HttpRequestParser m_pParser;
// };

struct ConnectionSession
{
    std::unique_ptr<AsyncIO::TCPClientSocket> m_pSocketConnection;
    unsigned long long m_ullConnectionID;
    HttpRequestParser m_pParser;
};

std::unordered_map<unsigned long long, std::unique_ptr<ConnectionSession>> connections;
unsigned long long connectionsCounter = 1;
std::string html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>My Async Server</title>"
    "<link rel=\"icon\" href=\"/favicon.ico\">"
    "<link rel=\"stylesheet\" href=\"/style.css\">"
    "</head>"
    "<body>"
    "<h1>Hello from my C++ server!</h1>"
    "<p>This page generates multiple HTTP requests.</p>"
    "<img src=\"/image.png\">"
    "<img src=\"/image2.png\">"
    "<script src=\"/app.js\"></script>"
    "</body>"
    "</html>";

std::string response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: " +
    std::to_string(html.size()) + "\r\n"
                                  "Connection: keep-alive\r\n"
                                  "\r\n" +
    html;

int main()
{
    int port = 9095;
    AsyncIO::EventLoop loop;
    auto socketRes = AsyncIO::TCPServerSocket::Create(&loop);
    AsyncIO::TCPServerSocket &serverSocket = socketRes.second;

    auto OnDataAvailableFunc = [&](AsyncIO::TCPClientSocket &socket, ConnectionSession *session)
    {
        std::cerr << "Data Available on socket with id, and session id " << socket.GetID() << ", " << session->m_ullConnectionID << std::endl;
        auto oRes = session->m_pParser.Parse<AsyncIO::TCPClientSocket>(socket);
        if (oRes.m_eStatus == HttpRequestParser::ParsingStatus::Failed)
        {
            std::cerr << "Invalid HttpRequest Recieved " << oRes.m_strMessage << std::endl;
            socket.Close();
            return;
        }
        else if (oRes.m_eStatus == HttpRequestParser::ParsingStatus::InProgress)
        {
            std::cerr << "Parsing is still in progress" << std::endl;
        }
        else
        {
            std::cerr << "parsing finished " << std::endl;
            socket.WriteAll(response.data(), response.size(), []()
                            { std::cerr << "Response written back" << std::endl; });
        }
    };

    auto onCloseFunc = []()
    {
        std::cerr << "Connection closed " << std::endl;
    };

    auto onAcceptFunc = [&](std::unique_ptr<AsyncIO::TCPClientSocket> newConnection)
    {
        std::cerr << "Connection accepted " << std::endl;

        std::unique_ptr<ConnectionSession> session = std::unique_ptr<ConnectionSession>(new ConnectionSession);
        newConnection->OnDataAvailable(std::bind(OnDataAvailableFunc, std::placeholders::_1, session.get()));
        newConnection->OnClose(onCloseFunc);
        session->m_pSocketConnection = std::move(newConnection);
        session->m_ullConnectionID = connectionsCounter;

        connections.insert({
            connectionsCounter,
            std::move(session),
        });
        ++connectionsCounter;
    };

    serverSocket.OnAccept(std::move(onAcceptFunc));

    auto result = serverSocket.Listen(port);
    if (!result.success)
    {
        std::cerr << "Error in listening to port " << port << std::endl;
        return -1;
    }
    std::cerr << "Server started listening on port " << port << std::endl;
    loop.Run();

    return 0;
}