#pragma once

//! System Includes
#include <functional>

//! Async Engine
#include "AsyncFdIO.h"

namespace AsyncIO
{
    class RunTime;

    class TerminalIO
    {
    public:
        explicit TerminalIO(RunTime *p_pLoop);

        //! CallBack APIs
        bool WriteAll(char *buffer, unsigned int size, std::function<void(void)> p_fOnCompletion = []() {});
        void OnRead(std::function<void(char *, unsigned int)> p_fOnReadCallback);
        void OnDataAvailable(std::function<void(TerminalIO &)>); //! Use either onDataAvailable cb or onRead cb whichever is set last

        //! Synchrnous APIs
        int ReadSync(char *buffer, unsigned int size);

        void OnClose(std::function<void(void)> p_fOnCloseCallback);
        void Close();

    private:
        AsyncFdIO m_oStdIn;
        AsyncFdIO m_oStdOut;
    };
}