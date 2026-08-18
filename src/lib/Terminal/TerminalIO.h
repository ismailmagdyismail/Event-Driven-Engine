#pragma once

//! System Includes
#include <functional>

//! Async Engine
#include "AsyncFdIO.h"

namespace AsyncIO
{
    class EventLoop;

    class TerminalIO
    {
    public:
        explicit TerminalIO(EventLoop *p_pLoop);

        bool WriteAll(char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion = []() {});
        void OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback);
        void OnClose(std::function<void(void)> p_fOnCloseCallback);
        void Close();

    private:
        AsyncFdIO m_oStdIn;
        AsyncFdIO m_oStdOut;
    };
}