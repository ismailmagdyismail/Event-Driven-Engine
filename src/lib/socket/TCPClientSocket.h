#pragma once

//! System Includes
#include <utility>
#include <functional>
#include <memory>

//! Async Engine
#include "Result.h"
#include "TCPSocket.h"
#include "EventLoop.h"
#include "AsyncFdIO.h"

namespace AsyncIO
{
    class EventLoop;
    class TCPServerSocket;
    class TCPClientSocket
    {
    public:
        TCPClientSocket(EventLoop *);
        TCPClientSocket(EventLoop *, SocketInfo);
        static std::pair<Result, TCPClientSocket> Create(EventLoop *);
        static TCPClientSocket Create(EventLoop *, SocketInfo);

        TCPClientSocket(const TCPClientSocket &) = delete;
        TCPClientSocket &operator=(const TCPClientSocket &) = delete;
        TCPClientSocket(TCPClientSocket &&) = delete;
        TCPClientSocket &operator=(TCPClientSocket &&) = delete;

        //! Callback APIs
        bool WriteAll(const char *buffer, unsigned int, std::function<void(void)> p_fOnCompletion = []() {});
        void OnDataAvailable(std::function<void(TCPClientSocket &)>); //! Use either onDataAvailable cb or onRead cb whichever is set last
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);

        //! Synchrnous APIs
        int ReadSync(char *buffer, unsigned int size);

        Result Connect(unsigned int port);
        int GetID();
        void Close();

    private:
        SocketInfo m_oSocketInfo;
        AsyncFdIO m_oAsyncFDIO;
    };
}