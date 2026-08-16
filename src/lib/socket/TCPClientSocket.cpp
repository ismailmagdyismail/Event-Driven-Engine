//! System Includes
#include <unistd.h>
#include <errno.h>

//! Async Engine
#include "TCPClientSocket.h"
#include "EventLoop.h"
#include "Events.h"
#include "PollUtils.h"

std::pair<AsyncIO::Result, AsyncIO::TCPClientSocket> AsyncIO::TCPClientSocket::Create(EventLoop *p_pEventLoop)
{
    std::pair<bool, AsyncIO::SocketInfo> socketCreationResult = AsyncIO::CreateTCPSocket();
    AsyncIO::Result result;
    if (!socketCreationResult.first)
    {
        result.success = false;
        result.message = "Failed To Create Server Socket";
        return {result, AsyncIO::TCPClientSocket{nullptr}};
    }
    AsyncIO::TCPClientSocket oSocket{p_pEventLoop};
    oSocket.m_oSocketInfo = socketCreationResult.second;
    oSocket.m_oAsyncFDIO.SetFD(oSocket.m_oSocketInfo.fd);
    result.success = true;
    return {
        result,
        oSocket,
    };
}

AsyncIO::TCPClientSocket AsyncIO::TCPClientSocket::Create(EventLoop *p_pEventLoop, SocketInfo socketInfo)
{
    AsyncIO::TCPClientSocket clientSocket{p_pEventLoop};
    clientSocket.m_oSocketInfo = std::move(socketInfo);
    clientSocket.m_oAsyncFDIO.SetFD(clientSocket.m_oSocketInfo.fd);
    return clientSocket;
}

AsyncIO::Result AsyncIO::TCPClientSocket::Connect(unsigned int port)
{
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    int connectionStatus = connect(m_oSocketInfo.fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (connectionStatus == -1 && errno != EINPROGRESS)
    {
        return Result{
            .success = false,
            .message = "Client failed to connect",
        };
    }
    return Result{
        .success = true,
        .message = "",
    };
}

void AsyncIO::TCPClientSocket::Write(const char *buffer, unsigned int size)
{
    m_oAsyncFDIO.Write(buffer, size);
}

void AsyncIO::TCPClientSocket::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    m_oAsyncFDIO.OnRead(std::move(p_fOnReadCallback));
}

void AsyncIO::TCPClientSocket::OnClose(std::function<void(void)> p_fOnCloseCallback)
{
    m_oAsyncFDIO.OnClose(std::move(p_fOnCloseCallback));
}

int AsyncIO::TCPClientSocket::GetID()
{
    return m_oAsyncFDIO.GetID();
}

AsyncIO::TCPClientSocket::TCPClientSocket(AsyncIO::EventLoop *p_pEventLoop)
    : m_oAsyncFDIO(p_pEventLoop)
{
}

void AsyncIO::TCPClientSocket::Close()
{
    m_oAsyncFDIO.Close();
}