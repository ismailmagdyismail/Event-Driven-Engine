//! System Includes
#include <unistd.h>

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

    //! TODO: WHY DOES THIS MAKE CLIENT Connect return -1
    // if (!AsyncIO::MakeNonBlocking(oSocket.m_oSocketInfo.fd))
    // {
    //     result.success = false;
    //     result.message = "Failed to make Server Socket Non Blocking";
    //     return {result, AsyncIO::TCPClientSocket{}};
    // }

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
    return clientSocket;
}

AsyncIO::Result AsyncIO::TCPClientSocket::Connect(unsigned int port)
{
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    int connectionStatus = connect(m_oSocketInfo.fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (connectionStatus == -1)
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

void AsyncIO::TCPClientSocket::HandleClientSocketReady(AsyncIO::EventContext ctx)
{
    if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, POLLHUP))
    {
        HandleClose();
    }
    else if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, POLLIN))
    {
        HandleClientDataReady();
    }
    else if (AsyncIO::PollHelpers::IsEventSet(ctx.readyEvents, POLLOUT))
    {
        throw std::runtime_error("[NOT-Implemented] Client socket space available");
        //! TODO:
        // HandleClientSpaceAvailable(ctx.readyEvents);
    }
}

void AsyncIO::TCPClientSocket::HandleClientDataReady()
{
    unsigned int size = 1024;
    char buffer[size];
    int readBytes = read(GetID(), buffer, size);
    if (readBytes == -1)
    {
        throw std::runtime_error("[FATAL]: Error In Reading Client Bytes for socket FD " + std::to_string(GetID()));
    }
    else if (readBytes == 0)
    {
        throw std::runtime_error("[FATAL]: connection closed by socket.read which is unxpected, event loop handle it with higher priority than read!");
    }
    if (m_fOnReadHandler)
    {
        m_fOnReadHandler(buffer, readBytes);
    }
}

void AsyncIO::TCPClientSocket::HandleClose()
{
    m_pEventLoop->UnRegisterFromAllEvents(GetID());
    if (m_fOnCloseHandler)
    {
        m_fOnCloseHandler();
    }
}

void AsyncIO::TCPClientSocket::OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback)
{
    SetupWithEventLoop();
    m_fOnReadHandler = std::move(p_fOnReadCallback);
}

void AsyncIO::TCPClientSocket::OnClose(std::function<void(void)> p_fOnCloseCallback)
{
    SetupWithEventLoop();
    m_fOnCloseHandler = std::move(p_fOnCloseCallback);
}

void AsyncIO::TCPClientSocket::SetupWithEventLoop()
{
    if (!m_bSetUp)
    {
        m_pEventLoop->SubScribeToEvent(GetID(), AsyncIO::EventType::Read, [this](AsyncIO::EventContext ctx)
                                       { HandleClientSocketReady(ctx); });
        m_bSetUp = true;
    }
}

int AsyncIO::TCPClientSocket::GetID()
{
    return m_oSocketInfo.fd;
}

AsyncIO::TCPClientSocket::TCPClientSocket(AsyncIO::EventLoop *p_pEventLoop)
{
    m_pEventLoop = p_pEventLoop;
}

void AsyncIO::TCPClientSocket::Close()
{
    close(GetID());
}