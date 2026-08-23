#pragma once

//! System Includes
#include <functional>
#include <unordered_map>

//! Async Engine Includes
#include "IEventLoopRegisterationHander.h"

/*
 - to support old callback push mechanism
 - supports push only
 - no yielding / future / promise mechanism
 - may get deprecated in future
*/

namespace AsyncIO
{

    struct EventLoopRegistery;

    class CallbackSubscribeHandler : public IEventLoopSubscriptionHandler
    {
    public:
        struct CallbackSubscribeHandlerContext
        {
            int fd;
            std::function<void(EventContext)> callback;
            short eventsToSubTo;
        };

        explicit CallbackSubscribeHandler(CallbackSubscribeHandlerContext);
        void HandleSubscriptionRequest(EventLoopRegistery &) override;

    private:
        CallbackSubscribeHandlerContext m_oContext;
    };

    class CallbackUnSubscribeHandler : public IEventLoopSubscriptionHandler
    {
    public:
        struct CallbackUnSubscribeHandlerContext
        {
            int fd;
        };

        explicit CallbackUnSubscribeHandler(CallbackUnSubscribeHandlerContext);
        void HandleSubscriptionRequest(EventLoopRegistery &) override;

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
