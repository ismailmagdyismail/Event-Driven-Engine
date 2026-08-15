#pragma once

//! System Includes
#include <utility>
#include <functional>

//! Async Engine
#include "Result.h"
#include "AsyncFdIO.h"

namespace AsyncIO
{
    class EventLoop;

    class TerminalIO
    {
    public:
        enum class TerminalType
        {
            STDIN,
            STDOUT,
            STDERR,
        };
        TerminalIO(TerminalType, EventLoop *);

        void Write(char *buffer, unsigned int);
        void OnRead(std::function<void(char *, unsigned int)>);
        void OnClose(std::function<void(void)>);
        int GetID();
        void Close();

    private:
        TerminalType m_eType;
        AsyncFdIO m_oAsyncFDIO;
    };
}