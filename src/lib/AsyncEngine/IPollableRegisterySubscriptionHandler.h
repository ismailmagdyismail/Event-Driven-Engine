#pragma once

namespace AsyncIO
{
    struct EventContext
    {
        int id;
        short readyEvents;
    };
    struct PollableRegistery;
    class IPollableRegisterySubscriptionHandler
    {
    public:
        virtual ~IPollableRegisterySubscriptionHandler() = default;
        virtual void HandleSubscriptionRequest(PollableRegistery &) = 0;
    };
}