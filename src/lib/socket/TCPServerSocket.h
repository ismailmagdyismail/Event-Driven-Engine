#pragma once

//! System Includes
#include <utility>
#include <arpa/inet.h>
#include <functional>

#include "Result.h"
#include "TCPSocket.h"
#include "TCPClientSocket.h"

namespace AsyncIO
{
    class EventLoop;
    class TCPServerSocket
    {
    public:
        static std::pair<Result, TCPServerSocket> Create(EventLoop *);

        Result Listen(unsigned int port);
        std::pair<Result, SocketInfo> AcceptSync();
        void OnAccept(std::function<void(AsyncIO::TCPClientSocket)> p_fOnAcceptCallback);
        int GetID();
        void Close();

    private:
        TCPServerSocket(EventLoop *);

        SocketInfo m_oSocketData;
        sockaddr_in m_oAddress;
        unsigned int m_uiPort;
        EventLoop *m_pEventLoop;
    };
}