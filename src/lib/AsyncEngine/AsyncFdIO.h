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
        void SetFD(int);

        void Write(char *buffer, unsigned int);
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);
        int GetID();
        void Close();

    private:
        void SetupWithEventLoop();

        void HandleEventReady(EventContext ctx);
        void HandleClose();
        void HandleDataReady();

        std::function<void(void)> m_fOnCloseHandler{nullptr};
        std::function<void(char *, unsigned int)> m_fOnReadHandler{nullptr};
        bool m_bSetUp{false};
        int m_iFD;
        EventLoop *m_pEventLoop;
    };
}