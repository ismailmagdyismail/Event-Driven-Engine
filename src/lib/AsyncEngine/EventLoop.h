#pragma once

//! System Includes
#include <atomic>
#include <poll.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

//! Async Engine
#include "Result.h"
#include "MPSCQueue.h"

namespace AsyncIO
{
    struct EventContext
    {
        int id;
        short readyEvents;
    };
    struct EventLoopRegistery
    {
        //! A Page Table / Buffer Pool inspired design
        //! Index + actual data

        std::unordered_map<int, unsigned int> m_mapMonitoredFDsIndex;
        std::vector<pollfd> m_vecMonitoredFDs;
        std::unordered_map<int, std::function<void(EventContext)>> m_mapCallBacks;
        // std::unordered_map<int, std::unordered_map<int, std::function<void(EventContext)>>> m_mapCallBacks;
    };

    class IEventLoopSubscriptionHandler
    {
    public:
        virtual ~IEventLoopSubscriptionHandler() = default;
        virtual void HandleSubscriptionRequest(EventLoopRegistery &) = 0;

    private:
    };

    class SubscribeHandler : public IEventLoopSubscriptionHandler
    {
    public:
        struct SubscribeHandlerContext
        {
            int fd;
            std::function<void(EventContext)> callback;
            short eventsToSubTo;
        };
        SubscribeHandler(SubscribeHandlerContext);
        void HandleSubscriptionRequest(EventLoopRegistery &) override;

    private:
        SubscribeHandlerContext m_oContext;
    };

    class UnSubscribeHandler : public IEventLoopSubscriptionHandler
    {
    public:
        struct UnSubscribeHandlerContext
        {
            int fd;
        };
        UnSubscribeHandler(UnSubscribeHandlerContext);
        void HandleSubscriptionRequest(EventLoopRegistery &) override;

    private:
        UnSubscribeHandlerContext m_oContext;
    };

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
