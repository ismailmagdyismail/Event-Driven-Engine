//! System Includes
#include <unistd.h>
#include <arpa/inet.h>
#include <tuple>

//! Async Engine
#include "TCPServerSocket.h"
#include "TCPSocket.h"
#include "RunTime.h"
#include "Events.h"
#include "AcceptFuture.h"

std::pair<AsyncIO::Result, AsyncIO::TCPServerSocket> AsyncIO::TCPServerSocket::Create(AsyncIO::RunTime *p_pEventLoop)
{
    //! 1. Create Socket FD
    std::pair<bool, AsyncIO::SocketInfo> socketCreationResult = AsyncIO::CreateTCPSocket();
    AsyncIO::Result result;
    if (!socketCreationResult.first)
    {
        result.success = false;
        result.message = "Failed To Create Server Socket";
        return std::pair<AsyncIO::Result, AsyncIO::TCPServerSocket>{
            std::piecewise_construct,
            std::forward_as_tuple(result),
            std::forward_as_tuple(nullptr),
        };
    }

    // 2. make it non blocking so reads, writes (if buffers are full) don't block caller threads.
    if (!AsyncIO::MakeNonBlocking(socketCreationResult.second.fd))
    {
        close(socketCreationResult.second.fd);
        result.success = false;
        result.message = "Failed to make Server Socket Non Blocking";
        return std::pair<AsyncIO::Result, AsyncIO::TCPServerSocket>{
            std::piecewise_construct,
            std::forward_as_tuple(result),
            std::forward_as_tuple(nullptr),
        };
    }

    result.success = true;
    return std::pair<AsyncIO::Result, AsyncIO::TCPServerSocket>{
        std::piecewise_construct,
        std::forward_as_tuple(result),
        std::forward_as_tuple(p_pEventLoop, socketCreationResult.second),
    };
}

AsyncIO::TCPServerSocket::TCPServerSocket(AsyncIO::RunTime *p_pEventloop)
{
    m_pEventLoop = p_pEventloop;
}

AsyncIO::TCPServerSocket::TCPServerSocket(AsyncIO::RunTime *p_pEventloop, AsyncIO::SocketInfo socketInfo)
    : m_oSocketData(socketInfo), m_pEventLoop(p_pEventloop)
{
}

AsyncIO::Result AsyncIO::TCPServerSocket::Listen(unsigned int port)
{
    //! 3. Bind
    m_oAddress = AsyncIO::CreateLocalAddress(port);

    int bindStatus = bind(m_oSocketData.fd, reinterpret_cast<sockaddr *>(&m_oAddress), sizeof(m_oAddress));
    if (bindStatus != 0)
    {
        return {
            false,
            "Error Binding Server Socket",
        };
    }
    int listenStatus = listen(m_oSocketData.fd, 0);
    if (listenStatus != 0)
    {
        return {
            false,
            "Error Listening on Server Socket with port " + std::to_string(port),
        };
    }
    m_uiPort = port;

    return AsyncIO::Result{
        .success = true,
        .message = std::string{},
    };
}

std::pair<AsyncIO::Result, AsyncIO::SocketInfo> AsyncIO::TCPServerSocket::AcceptSync()
{
    AsyncIO::Result result;
    unsigned int addressSize = sizeof(m_oAddress);
    int clientFD = accept(m_oSocketData.fd, reinterpret_cast<sockaddr *>(&m_oAddress), &addressSize);
    if (clientFD != -1)
    {
        AsyncIO::MakeNonBlocking(clientFD);
        return {
            AsyncIO::Result{.success = true},
            AsyncIO::SocketInfo{.fd = clientFD},
        };
    }
    else
    {
        return {
            AsyncIO::Result{.success = false, .message = "Failed To Accept Client Connection"},
            AsyncIO::SocketInfo{},
        };
    }
}

AsyncIO::AcceptFuture *AsyncIO::TCPServerSocket::Accept()
{
    return AcceptFuture::Spawn(m_oSocketData.fd, m_pEventLoop);
}

void AsyncIO::TCPServerSocket::OnAccept(std::function<void(std::unique_ptr<AsyncIO::TCPClientSocket>)> p_fOnAcceptCallback)
{
    auto callback = [this, onAccCallback = std::move(p_fOnAcceptCallback)](AsyncIO::EventContext)
    {
        auto result = AcceptSync();
        if (!result.first.success)
        {
            throw std::runtime_error("[FATAL]: Unhandled OnAcceptingError");
        }
        AsyncIO::SocketInfo sockInfo{
            .fd = result.second.fd,
        };
        std::unique_ptr<AsyncIO::TCPClientSocket> clientSocket{new AsyncIO::TCPClientSocket(m_pEventLoop, sockInfo)};
        onAccCallback(std::move(clientSocket));
    };

    m_pEventLoop->SubScribeToEvent(GetID(), AsyncIO::EventType::Read, std::move(callback));
}

int AsyncIO::TCPServerSocket::GetID()
{
    return m_oSocketData.fd;
}

void AsyncIO::TCPServerSocket::Close()
{
    if (m_oSocketData.fd == -1)
    {
        return;
    }
    m_oSocketData.fd = -1;
    m_pEventLoop->UnRegisterFromAllEvents(GetID());
    close(GetID());
}

AsyncIO::TCPServerSocket::~TCPServerSocket()
{
    Close();
}