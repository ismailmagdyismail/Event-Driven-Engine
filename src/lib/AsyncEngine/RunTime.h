#pragma once

//! System Includes
#include <atomic>
#include <functional>
#include <memory>

//! Async Engine
#include "PollableRegistery.h"
#include "Result.h"
#include "MPSCQueue.h"

namespace AsyncIO
{
    class IFuture;
    class RunTime
    {
    public:
        RunTime();

        Result Run();
        void Stop();

        void SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(EventContext)> &&callback);
        void UnRegisterFromAllEvents(int id);
        void AddMainTask(std::function<void(void)> p_fMainTask);
        void RegisterFuture(int id, short eventsToSubTo, IFuture *p_pFuture);
        void UnRegisterFutureFromAllEvents(int id, IFuture *p_pFuture);

    private:
        void DispatchToFuture(pollfd &monitoredFd);
        void PollFuture(IFuture *p_pFuture, int fd, short event, short revents);
        void RemoveFuture(int fd, short updatedEventMask, AsyncIO::IFuture *p_pFuture);

        std::function<void(void)> m_fMainTask{nullptr};
        MPSCQueue<std::unique_ptr<IPollableRegisterySubscriptionHandler>> m_oRequestQueue;
        PollableRegistery m_oRegistery;
        std::atomic<bool> m_bRunning;
    };
}
