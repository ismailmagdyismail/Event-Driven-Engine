//! System Includes
#include <unistd.h>
#include <errno.h>
#include <tuple>

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
        return std::pair<AsyncIO::Result, AsyncIO::TCPClientSocket>{
            std::piecewise_construct,
            std::forward_as_tuple(result),
            std::forward_as_tuple(nullptr),
        };
    }
    result.success = true;
    return std::pair<AsyncIO::Result, AsyncIO::TCPClientSocket>{
        std::piecewise_construct,
        std::forward_as_tuple(result),
        std::forward_as_tuple(p_pEventLoop, socketCreationResult.second),
    };
}

AsyncIO::TCPClientSocket AsyncIO::TCPClientSocket::Create(EventLoop *p_pEventLoop, SocketInfo socketInfo)
{
    return AsyncIO::TCPClientSocket{p_pEventLoop, socketInfo};
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

bool AsyncIO::TCPClientSocket::WriteAll(const char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion)
{
    return m_oAsyncFDIO.WriteAll(buffer, size, std::move(p_fOnCompletion));
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

AsyncIO::TCPClientSocket::TCPClientSocket(AsyncIO::EventLoop *p_pEventLoop, AsyncIO::SocketInfo socketInfo)
    : m_oSocketInfo(socketInfo), m_oAsyncFDIO(p_pEventLoop)
{
    m_oAsyncFDIO.SetFD(m_oSocketInfo.fd);
}

void AsyncIO::TCPClientSocket::Close()
{
    m_oAsyncFDIO.Close();
}