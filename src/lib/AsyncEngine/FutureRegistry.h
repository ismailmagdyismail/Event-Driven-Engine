#pragma once

//! Async Engine Includes
#include "IPollableRegisterySubscriptionHandler.h"

//! System Includes
#include <unordered_map>

namespace AsyncIO
{
    struct EventContext;
    struct PollableRegistery;
    class IFuture;

    class FutureSubscriptionHandler : public IPollableRegisterySubscriptionHandler
    {
    public:
        struct FutureSubscriptionHandlerContext
        {
            int fd;
            short eventsToSubTo;
            IFuture *p_pFuture;
        };
        FutureSubscriptionHandler(FutureSubscriptionHandlerContext context);
        void HandleSubscriptionRequest(PollableRegistery &) override;

    private:
        FutureSubscriptionHandlerContext m_oContext;
    };

    class FutureUnSubscriptionHandler : public IPollableRegisterySubscriptionHandler
    {
    public:
        struct FutureUnSubscriptionHandlerContext
        {
            int fd;
            IFuture *p_pFuture;
        };
        FutureUnSubscriptionHandler(FutureUnSubscriptionHandlerContext context);
        void HandleSubscriptionRequest(PollableRegistery &) override;

    private:
        FutureUnSubscriptionHandlerContext m_oContext;
    };

    class FutureRegistry
    {
    public:
        void Set(int fd, short eventsToSubTo, IFuture *p_pFuture);
        void RemoveAll(int fd, IFuture *p_pFuture);
        IFuture *GetFuture(int fd, short event) const;

    private:
        std::unordered_map<int, std::unordered_map<short, IFuture *>> m_mapFutures;
    };
}