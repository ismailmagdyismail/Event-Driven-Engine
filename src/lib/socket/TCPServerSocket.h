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
    class RunTime;
    class AcceptFuture;
    class TCPServerSocket
    {
    public:
        static std::pair<Result, TCPServerSocket> Create(RunTime *);
        TCPServerSocket(RunTime *);
        TCPServerSocket(RunTime *, SocketInfo);
        ~TCPServerSocket();

        TCPServerSocket(const TCPServerSocket &) = delete;
        TCPServerSocket &operator=(const TCPServerSocket &) = delete;
        TCPServerSocket(TCPServerSocket &&) = delete;
        TCPServerSocket &operator=(TCPServerSocket &&) = delete;

        Result Listen(unsigned int port);

        //! Callbacks
        void OnAccept(std::function<void(std::unique_ptr<AsyncIO::TCPClientSocket>)> p_fOnAcceptCallback);

        //! Async APIs
        AcceptFuture *Accept();

        int GetID();
        void Close();

    private:
        std::pair<Result, SocketInfo> AcceptSync();
        SocketInfo m_oSocketData;
        sockaddr_in m_oAddress;
        unsigned int m_uiPort;
        RunTime *m_pEventLoop;
    };
}