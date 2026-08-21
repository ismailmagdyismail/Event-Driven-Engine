//! System Includes
#include <iostream>
#include <string_view>
#include <vector>

#include "EventLoop.h"
#include "TCPServerSocket.h"
#include "HttpRequestParser.h"
#include "TerminalIO.h"

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
            std::cerr << "PAYLOAD " << std::string_view(oRes.m_oRequest.m_bufferData, oRes.m_oRequest.m_uiSize) << std::endl;
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