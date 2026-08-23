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
    class RunTime
    {
    public:
        RunTime();

        Result Run();
        void Stop();

        void SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(EventContext)> &&callback);
        void UnRegisterFromAllEvents(int id);
        void AddMainTask(std::function<void(void)> p_fMainTask);

    private:
        std::function<void(void)> m_fMainTask{nullptr};
        MPSCQueue<std::unique_ptr<IPollableRegisterySubscriptionHandler>> m_oRequestQueue;
        PollableRegistery m_oRegistery;
        std::atomic<bool> m_bRunning;
    };
}
