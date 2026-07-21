#pragma once

//! System Includes
#include <atomic>
#include <poll.h>
#include <functional>

//! Async Engine
#include "Result.h"

namespace AsyncIO
{
    struct EventContext
    {
        int id;
        short readyEvents;
    };

    class EventLoop
    {
    public:
        EventLoop();

        Result Run();
        void Stop();

        void SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(EventContext)> &&callback);
        void UnRegisterFromAllEvents(int id);

    private:
        std::vector<pollfd> m_vecMonitoredFDs;
        std::atomic<bool> m_bRunning;
        std::unordered_map<int, std::function<void(EventContext)>> m_mapCallBacks;
        // std::unordered_map<int, std::unordered_map<int, std::function<void(EventContext)>>> m_mapCallBacks;
    };
}
