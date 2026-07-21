//! System Includes
#include <unistd.h>
#include <arpa/inet.h>

//! Async Engine
#include "TCPServerSocket.h"
#include "TCPSocket.h"

std::pair<AsyncIO::Result, AsyncIO::TCPServerSocket> AsyncIO::TCPServerSocket::Create()
{
    //! 1. Create Socket FD
    std::pair<bool, AsyncIO::SocketInfo> socketCreationResult = AsyncIO::CreateTCPSocket();
    AsyncIO::Result result;
    if (!socketCreationResult.first)
    {
        result.success = false;
        result.message = "Failed To Create Server Socket";
        return {result, AsyncIO::TCPServerSocket{}};
    }
    AsyncIO::TCPServerSocket oSocket;
    oSocket.m_oSocketData = socketCreationResult.second;

    // 2. make it non blocking so reads, writes (if buffers are full) don't block caller threads.
    if (!AsyncIO::MakeNonBlocking(oSocket.m_oSocketData.fd))
    {
        result.success = false;
        result.message = "Failed to make Server Socket Non Blocking";
        return {result, AsyncIO::TCPServerSocket{}};
    }

    result.success = true;
    return {
        result,
        oSocket,
    };
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

    return {
        true,
    };
}

std::pair<AsyncIO::Result, AsyncIO::SocketInfo> AsyncIO::TCPServerSocket::Accept()
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

AsyncIO::SocketInfo AsyncIO::TCPServerSocket::GetSocketInfo()
{
    return m_oSocketData;
}