#pragma once

//! Async Engine Includes
#include "Result.h"
#include "IFuture.h"

namespace AsyncIO
{
    class TCPClientSocket;
    class RunTime;

    class ReadFuture : public BaseFuture<std::pair<char *, unsigned int>>
    {
    public:
        ReadFuture() = default;
        ReadFuture(int fd, RunTime *p_pEventLoop, char *buffer, unsigned int size);
        ~ReadFuture() override;

        static ReadFuture *Spawn(int fd, RunTime *p_pRunTime, char *buffer, unsigned int size);

        ReadFuture(const ReadFuture &) = delete;
        ReadFuture &operator=(const ReadFuture &) = delete;
        ReadFuture(ReadFuture &&) = delete;
        ReadFuture &operator=(ReadFuture &&) = delete;

        FutureStatus Poll() override;
        void AttachToRunTime();

    private:
        std::pair<char *, unsigned int> GetValue() override;

        RunTime *m_pEventLoop{nullptr};
        int m_fd{-1};
        int m_iErrorCode{0};
        int m_iBytesRead{0};
        char *m_buffer{nullptr};
        unsigned int m_uisize{0};
    };
}