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

        bool WriteAll(const char *buffer, unsigned int, std::function<void(void)> p_fOnCompletion = []() {});
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);
        int GetID();
        void Close();

    private:
        void WriteFromPendingBuffer();
        void ResetPendingBuffer();
        void HandleEventReady(EventContext ctx);
        void HandleClose();
        void HandleDataReady();

        const char *m_pendingBuffer{nullptr};
        unsigned int m_uiPendingBufferSize{0};
        unsigned int m_uiPendingBufferOffset{0};
        std::function<void(void)> m_fOnWriteComplete{nullptr};
        std::function<void(void)> m_fOnCloseHandler{nullptr};
        std::function<void(char *, unsigned int)> m_fOnReadHandler{nullptr};
        int m_iFD{-1};
        EventLoop *m_pEventLoop{nullptr};
    };
}