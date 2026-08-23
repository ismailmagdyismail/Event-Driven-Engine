#pragma once

//! System Includes
#include <functional>
#include <unordered_map>

//! Async Engine Includes
#include "IPollableRegisterySubscriptionHandler.h"

/*
 - to support old callback push mechanism
 - supports push only
 - no yielding / future / promise mechanism
 - may get deprecated in future
*/

namespace AsyncIO
{

    struct PollableRegistery;

    class CallbackSubscribeHandler : public IPollableRegisterySubscriptionHandler
    {
    public:
        struct CallbackSubscribeHandlerContext
        {
            int fd;
            std::function<void(EventContext)> callback;
            short eventsToSubTo;
        };

        explicit CallbackSubscribeHandler(CallbackSubscribeHandlerContext);
        void HandleSubscriptionRequest(PollableRegistery &) override;

    private:
        CallbackSubscribeHandlerContext m_oContext;
    };

    class CallbackUnSubscribeHandler : public IPollableRegisterySubscriptionHandler
    {
    public:
        struct CallbackUnSubscribeHandlerContext
        {
            int fd;
        };

        explicit CallbackUnSubscribeHandler(CallbackUnSubscribeHandlerContext);
        void HandleSubscriptionRequest(PollableRegistery &) override;

    private:
        CallbackUnSubscribeHandlerContext m_oContext;
    };

    struct CallbackRegistry
    {
        std::unordered_map<int, std::function<void(EventContext)>> m_mapCallBacks;

        bool Contains(int fd) const;
        void Set(int fd, std::function<void(EventContext)> callback);
        void Remove(int fd);
    };
}
