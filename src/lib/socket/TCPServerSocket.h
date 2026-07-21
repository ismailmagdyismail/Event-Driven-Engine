#pragma once

//! System Includes
#include <utility>
#include <arpa/inet.h>

#include "Result.h"
#include "TCPSocket.h"

namespace AsyncIO
{
    class TCPServerSocket
    {
    public:
        static std::pair<Result, TCPServerSocket> Create();

        Result Listen(unsigned int port);
        std::pair<Result, SocketInfo> Accept();
        int GetID();

    private:
        SocketInfo m_oSocketData;
        sockaddr_in m_oAddress;
        unsigned int m_uiPort;
    };
}