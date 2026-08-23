#pragma once

//! Async Engine Includes
#include "Result.h"
#include "IFuture.h"

namespace AsyncIO
{
    class TCPClientSocket;
    class RunTime;

    class AcceptFuture : public IFuture
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

        //! Continuations
        void Then(std::function<void(std::unique_ptr<AsyncIO::TCPClientSocket>)> callback);
        void Catch(std::function<void(Result)> callback);

    private:
        void AttachToRunTime();

        RunTime *m_pEventLoop{nullptr};
        int m_fd{-1};
        int m_iCreatedClientSocketFD{-1};
        int m_iErrorCode{0};
    };
}