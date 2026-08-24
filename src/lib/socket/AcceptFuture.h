#pragma once

//! Async Engine Includes
#include "Result.h"
#include "IFuture.h"

namespace AsyncIO
{
    class TCPClientSocket;
    class RunTime;

    class AcceptFuture : public BaseFuture<std::unique_ptr<TCPClientSocket>>
    {
    public:
        //! Init, De-Init
        AcceptFuture(int fd, RunTime *p_pEventLoop);
        ~AcceptFuture() override;

        static AcceptFuture *Spawn(int fd, RunTime *p_pRunTime);

        AcceptFuture(const AcceptFuture &) = delete;
        AcceptFuture &operator=(const AcceptFuture &) = delete;
        AcceptFuture(AcceptFuture &&) = delete;
        AcceptFuture &operator=(AcceptFuture &&) = delete;

        FutureStatus Poll() override;

    private:
        void AttachToRunTime();
        std::unique_ptr<TCPClientSocket> GetValue() override;

        RunTime *m_pEventLoop{nullptr};
        int m_fd{-1};
        int m_iCreatedClientSocketFD{-1};
        int m_iErrorCode{0};
    };
}