#pragma once

//! System Includes
#include <utility>

//! Async Engine
#include "Result.h"
#include "TCPSocket.h"

namespace AsyncIO
{
    class TCPClientSocket
    {
    public:
        static std::pair<Result, TCPClientSocket> Create();
        static void Create(SocketInfo);

        Result Connect(unsigned int port);
        int GetID();

    private:
        SocketInfo m_oSocketInfo;
    };
}