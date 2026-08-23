#pragma once

//! System Includes
#include <utility>
#include <functional>

//! Async Engine
#include "Result.h"
#include "TCPSocket.h"
#include "RunTime.h"

namespace AsyncIO
{
    class RunTime;
    class AsyncFdIO
    {
    public:
        AsyncFdIO(RunTime *);
        ~AsyncFdIO();

        AsyncFdIO(const AsyncFdIO &) = delete;
        AsyncFdIO &operator=(const AsyncFdIO &) = delete;
        AsyncFdIO(AsyncFdIO &&) = delete;
        AsyncFdIO &operator=(AsyncFdIO &&) = delete;

        void SetFD(int, short flagsToSubTo);

        //! Callback APIs
        bool WriteAll(const char *buffer, unsigned int, std::function<void(void)> p_fOnCompletion = []() {});
        void OnDataAvailable(std::function<void(void)>); //! Use either onDataAvailable cb or onRead cb whichever is set last
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);

        //! Synchronous APIs
        int ReadSync(char *buffer, unsigned int size);

        int GetID();
        void Close();

    private:
        void WriteFromPendingBuffer();
        void ResetPendingBuffer();
        void HandleEventReady(EventContext ctx);
        void HandleClose();
        void HandleReadData();
        void HandleReadEvent();
        bool ConnectionInterrupted(int writtenBytes, int errcode);
        bool BackPressure(int writtenBytes, int errcode);

        const char *m_pendingBuffer{nullptr};
        unsigned int m_uiPendingBufferSize{0};
        unsigned int m_uiPendingBufferOffset{0};
        std::function<void(void)> m_fOnWriteComplete{nullptr};
        std::function<void(void)> m_fOnCloseHandler{nullptr};
        std::function<void(char *, unsigned int)> m_fOnReadHandler{nullptr};
        std::function<void(void)> m_fOnDataAvailableHandler{nullptr};
        int m_iFD{-1};
        RunTime *m_pEventLoop{nullptr};
    };
}