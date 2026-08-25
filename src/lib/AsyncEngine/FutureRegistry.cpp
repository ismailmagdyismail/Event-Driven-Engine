//! Async Engine Includes
#include "FutureRegistry.h"
#include "PollableRegistery.h"
#include "PollUtils.h"
#include "IFuture.h"

AsyncIO::FutureSubscriptionHandler::FutureSubscriptionHandler(FutureSubscriptionHandlerContext context)
    : m_oContext(context)
{
}

void AsyncIO::FutureSubscriptionHandler::HandleSubscriptionRequest(AsyncIO::PollableRegistery &registery)
{
    registery.m_oFutureRegistry.Set(m_oContext.fd, m_oContext.eventsToSubTo, m_oContext.p_pFuture);
    registery.m_oMonitoredFdRegistry.AddOrUpdate(m_oContext.fd, m_oContext.eventsToSubTo);
}

AsyncIO::FutureUnSubscriptionHandler::FutureUnSubscriptionHandler(FutureUnSubscriptionHandlerContext context)
    : m_oContext(context)
{
}

void AsyncIO::FutureUnSubscriptionHandler::HandleSubscriptionRequest(AsyncIO::PollableRegistery &registery)
{
    registery.m_oMonitoredFdRegistry.Remove(m_oContext.fd);
    registery.m_oFutureRegistry.RemoveAll(m_oContext.fd, m_oContext.p_pFuture);
}

void AsyncIO::FutureRegistry::Set(int fd, short eventsToSubTo, IFuture *p_pFuture)
{
    std::vector<short> splittedEvents = AsyncIO::PollHelpers::GetActiveEvents(eventsToSubTo);
    for (const auto &event : splittedEvents)
    {
        m_mapFutures[fd][event] = p_pFuture;
    }
}

void AsyncIO::FutureRegistry::RemoveAll(int fd, IFuture *p_pFuture)
{
    auto it = m_mapFutures.find(fd);
    if (it == m_mapFutures.end())
    {
        return;
    }
    auto &eventsToFuture = it->second;
    for (auto eventIt = eventsToFuture.begin(); eventIt != eventsToFuture.end();)
    {
        if (eventIt->second == p_pFuture)
        {
            eventIt = eventsToFuture.erase(eventIt);
        }
        else
        {
            ++eventIt;
        }
    }
    if (eventsToFuture.empty())
    {
        m_mapFutures.erase(it);
    }
}

AsyncIO::IFuture *AsyncIO::FutureRegistry::GetFuture(int fd, short event) const
{
    auto it = m_mapFutures.find(fd);
    if (it == m_mapFutures.end())
    {
        return nullptr;
    }
    const auto &futures = it->second;
    auto futureIt = futures.find(event);
    if (futureIt == futures.end())
    {
        return nullptr;
    }
    return futureIt->second;
}