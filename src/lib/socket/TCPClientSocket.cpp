#include "TCPClientSocket.h"

std::pair<AsyncIO::Result, AsyncIO::TCPClientSocket> AsyncIO::TCPClientSocket::Create()
{
    std::pair<bool, AsyncIO::SocketInfo> socketCreationResult = AsyncIO::CreateTCPSocket();
    AsyncIO::Result result;
    if (!socketCreationResult.first)
    {
        result.success = false;
        result.message = "Failed To Create Server Socket";
        return {result, AsyncIO::TCPClientSocket{}};
    }
    AsyncIO::TCPClientSocket oSocket;
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

void AsyncIO::TCPClientSocket::Create(SocketInfo socketInfo)
{
    AsyncIO::TCPClientSocket clientSocket;
    clientSocket.m_oSocketInfo = std::move(socketInfo);
}

#include <iostream>
AsyncIO::Result AsyncIO::TCPClientSocket::Connect(unsigned int port)
{
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    int connectionStatus = connect(m_oSocketInfo.fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    std::cerr << m_oSocketInfo.fd << " = " << connectionStatus << std::endl;
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

int AsyncIO::TCPClientSocket::GetID()
{
    return m_oSocketInfo.fd;
}