//! System Includes
#include <poll.h>
#include <iostream>
#include <thread>
#include <chrono>

//! Async Engine Includes
#include "EventLoop.h"
#include "PollUtils.h"
#include "MPSCQueue.h"

AsyncIO::EventLoop::EventLoop() : m_oRequestQueue(1000)
{
    m_bRunning.store(false, std::memory_order_relaxed);
}

AsyncIO::Result AsyncIO::EventLoop::Run()
{
    m_bRunning.store(true, std::memory_order_relaxed);
    while (m_bRunning.load(std::memory_order_relaxed))
    {

        std::unique_ptr<IEventLoopSubscriptionHandler> handler;
        auto state = m_oRequestQueue.Pop(handler);
        switch (static_cast<int>(state))
        {
        case static_cast<int>(AsyncIO::MPSCQueueReadState::Stopped):
            m_bRunning.store(false, std::memory_order_relaxed);
            return AsyncIO::Result{.success = true, .message = "Event Loop Stopped Since Subscription Queue is Stopped"};
            break;
        case static_cast<int>(AsyncIO::MPSCQueueReadState::Success):
            handler->HandleSubscriptionRequest(m_oRegistery);
            break;
        default:
            break;
        }

        int iPollStatus = poll(m_oRegistery.m_vecMonitoredFDs.data(), m_oRegistery.m_vecMonitoredFDs.size(), 0);
        if (iPollStatus == -1)
        {
            m_bRunning.store(false, std::memory_order_relaxed);
            return AsyncIO::Result{.success = false, .message = "Error in polling Descriptors"};
        }
        //! Iterate over file desc being polled
        for (unsigned int i = 0; i < m_oRegistery.m_vecMonitoredFDs.size(); ++i)
        {
            if (AsyncIO::PollHelpers::IsReady(m_oRegistery.m_vecMonitoredFDs[i].revents))
            {
                auto callbackIt = m_oRegistery.m_mapCallBacks.find(m_oRegistery.m_vecMonitoredFDs[i].fd);
                if (callbackIt == m_oRegistery.m_mapCallBacks.end())
                {
                    continue;
                }
                callbackIt->second(EventContext{
                    .id = m_oRegistery.m_vecMonitoredFDs[i].fd,
                    .readyEvents = m_oRegistery.m_vecMonitoredFDs[i].revents,
                });
            }
        }
        std::this_thread::yield();
    }
    return AsyncIO::Result{.success = true};
}

void AsyncIO::EventLoop::SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(AsyncIO::EventContext)> &&callback)
{
    m_oRequestQueue.Push(std::make_unique<AsyncIO::SubscribeHandler>(SubscribeHandler::SubscribeHandlerContext{
        .callback = std::move(callback),
        .fd = id,
        .eventsToSubTo = eventsToSubscribeTo,
    }));
}

void AsyncIO::EventLoop::UnRegisterFromAllEvents(int id)
{
    m_oRequestQueue.Push(std::make_unique<AsyncIO::UnSubscribeHandler>(UnSubscribeHandler::UnSubscribeHandlerContext{
        .fd = id,
    }));
}

void AsyncIO::EventLoop::Stop()
{
    m_oRequestQueue.StopReading();
    m_oRequestQueue.StopWriting();
    m_bRunning.store(false, std::memory_order_relaxed);
}

AsyncIO::SubscribeHandler::SubscribeHandler(AsyncIO::SubscribeHandler::SubscribeHandlerContext context) : m_oContext(std::move(context))
{
}

void AsyncIO::SubscribeHandler::HandleSubscriptionRequest(AsyncIO::EventLoopRegistery &registery)
{
    registery.m_mapCallBacks[m_oContext.fd] = std::move(m_oContext.callback);

    auto monitoredFdIt = registery.m_mapMonitoredFDsIndex.find(m_oContext.fd);
    if (monitoredFdIt != registery.m_mapMonitoredFDsIndex.end())
    {
        registery.m_vecMonitoredFDs[monitoredFdIt->second].events =
            AsyncIO::PollHelpers::SetEvent(registery.m_vecMonitoredFDs[monitoredFdIt->second].events, m_oContext.eventsToSubTo);
        return;
    }

    pollfd monitoredFd{};
    monitoredFd.fd = m_oContext.fd;
    monitoredFd.events = m_oContext.eventsToSubTo;
    registery.m_vecMonitoredFDs.push_back(monitoredFd);
    registery.m_mapMonitoredFDsIndex[m_oContext.fd] = registery.m_vecMonitoredFDs.size() - 1;
}

AsyncIO::UnSubscribeHandler::UnSubscribeHandler(AsyncIO::UnSubscribeHandler::UnSubscribeHandlerContext context) : m_oContext(std::move(context))
{
}

void AsyncIO::UnSubscribeHandler::HandleSubscriptionRequest(AsyncIO::EventLoopRegistery &registery)
{
    auto monitoredFdIt = registery.m_mapMonitoredFDsIndex.find(m_oContext.fd);
    if (monitoredFdIt == registery.m_mapMonitoredFDsIndex.end())
    {
        return;
    }

    //! 1. Replace element to be removed with the last element
    //! 2. then remove last element (safe since already replicated by step 1.)
    //! this allows removal from back, avoid shifting all elements by 1 slot.
    unsigned int idx = monitoredFdIt->second;
    unsigned int lastIdx = registery.m_vecMonitoredFDs.size() - 1;
    if (idx != lastIdx)
    {
        registery.m_vecMonitoredFDs[idx] = registery.m_vecMonitoredFDs[lastIdx];
        registery.m_mapMonitoredFDsIndex[registery.m_vecMonitoredFDs[idx].fd] = idx;
    }

    registery.m_vecMonitoredFDs.pop_back();
    registery.m_mapMonitoredFDsIndex.erase(monitoredFdIt);
    registery.m_mapCallBacks.erase(m_oContext.fd);
}
