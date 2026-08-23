#pragma once

namespace AsyncIO
{
    struct EventContext
    {
        int id;
        short readyEvents;
    };
    struct EventLoopRegistery;
    class IEventLoopSubscriptionHandler
    {
    public:
        virtual ~IEventLoopSubscriptionHandler() = default;
        virtual void HandleSubscriptionRequest(EventLoopRegistery &) = 0;
    };
}