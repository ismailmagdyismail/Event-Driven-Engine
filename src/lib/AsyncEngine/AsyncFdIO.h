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
    class AsyncFdIO
    {
    public:
        AsyncFdIO(EventLoop *);
        ~AsyncFdIO();

        AsyncFdIO(const AsyncFdIO &) = delete;
        AsyncFdIO &operator=(const AsyncFdIO &) = delete;
        AsyncFdIO(AsyncFdIO &&) = delete;
        AsyncFdIO &operator=(AsyncFdIO &&) = delete;

        void SetFD(int);

        void Write(const char *buffer, unsigned int);
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);
        int GetID();
        void Close();

    private:
        void HandleEventReady(EventContext ctx);
        void HandleClose();
        void HandleDataReady();

        std::function<void(void)> m_fOnCloseHandler{nullptr};
        std::function<void(char *, unsigned int)> m_fOnReadHandler{nullptr};
        int m_iFD{-1};
        EventLoop *m_pEventLoop{nullptr};
    };
}