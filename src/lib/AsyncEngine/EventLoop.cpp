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
                m_oRegistery.m_mapCallBacks[m_oRegistery.m_vecMonitoredFDs[i].fd](EventContext{
                    .id = m_oRegistery.m_vecMonitoredFDs[i].fd,
                    .readyEvents = m_oRegistery.m_vecMonitoredFDs[i].revents,
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    registery.m_vecMonitoredFDs.push_back(pollfd{
        .events = m_oContext.eventsToSubTo,
        .fd = m_oContext.fd,
    });
    auto it = registery.m_vecMonitoredFDs.end();
    it--;
    int idx = registery.m_vecMonitoredFDs.size() - 1;
    registery.m_mapMonitoredFDsIndex[m_oContext.fd] = {idx, it};
}

AsyncIO::UnSubscribeHandler::UnSubscribeHandler(AsyncIO::UnSubscribeHandler::UnSubscribeHandlerContext context) : m_oContext(std::move(context))
{
}

void AsyncIO::UnSubscribeHandler::HandleSubscriptionRequest(AsyncIO::EventLoopRegistery &registery)
{
    auto it = registery.m_mapMonitoredFDsIndex[m_oContext.fd].second;
    registery.m_vecMonitoredFDs.erase(it);
    registery.m_mapCallBacks.erase(m_oContext.fd);
}
