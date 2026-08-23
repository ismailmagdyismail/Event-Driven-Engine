#pragma once

//! System Includes
#include <atomic>
#include <functional>
#include <memory>

//! Async Engine
#include "EventLoopRegistry.h"
#include "Result.h"
#include "MPSCQueue.h"

namespace AsyncIO
{
    class EventLoop
    {
    public:
        EventLoop();

        Result Run();
        void Stop();

        void SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(EventContext)> &&callback);
        void UnRegisterFromAllEvents(int id);
        void AddMainTask(std::function<void(void)> p_fMainTask);

    private:
        std::function<void(void)> m_fMainTask{nullptr};
        MPSCQueue<std::unique_ptr<IEventLoopSubscriptionHandler>> m_oRequestQueue;
        EventLoopRegistery m_oRegistery;
        std::atomic<bool> m_bRunning;
    };
}
