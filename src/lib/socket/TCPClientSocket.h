#pragma once

//! System Includes
#include <utility>
#include <functional>

//! Async Engine
#include "Result.h"
#include "TCPSocket.h"
#include "EventLoop.h"

namespace AsyncIO
{
    class EventLoop;
    class TCPClientSocket
    {
    public:
        static std::pair<Result, TCPClientSocket> Create(EventLoop *);
        static TCPClientSocket Create(EventLoop *, SocketInfo);

        Result Connect(unsigned int port);
        void Write(char *buffer, unsigned int);
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);
        int GetID();
        void Close();

    private:
        TCPClientSocket(EventLoop *);
        void SetupWithEventLoop();

        void HandleClientSocketReady(EventContext ctx);
        void HandleClose();
        void HandleClientDataReady();

        std::function<void(void)> m_fOnCloseHandler{nullptr};
        std::function<void(char *, unsigned int)> m_fOnReadHandler{nullptr};
        bool m_bSetUp{false};
        SocketInfo m_oSocketInfo;
        EventLoop *m_pEventLoop;
    };
}