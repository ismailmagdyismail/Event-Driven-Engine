//! System Includes
#include <poll.h>
#include <iostream>
#include <thread>
#include <chrono>

//! Async Engine Includes
#include "RunTime.h"
#include "PollableRegistery.h"
#include "PollUtils.h"
#include "MPSCQueue.h"

AsyncIO::RunTime::RunTime() : m_oRequestQueue(1000)
{
    m_bRunning.store(false, std::memory_order_relaxed);
}

void AsyncIO::RunTime::AddMainTask(std::function<void(void)> p_fMainTask)
{
    m_fMainTask = std::move(p_fMainTask);
}

AsyncIO::Result AsyncIO::RunTime::Run()
{
    m_bRunning.store(true, std::memory_order_relaxed);
    while (m_bRunning.load(std::memory_order_relaxed))
    {

        /*
            - execution of subscriptions / unsubscriptions is done async
            - published to MPSCQueue and handled in the event loop thread
            - we are doing this for now (since we don't know yet how / where will we add threads in future)
            - IFF we decided to continue single threaded only, we can remove
                - the MPSCQueue
                - the handle subscriptions / unsubscriptions handlers
                and instead call the registery directly (since the caller is guranteed to be in the same thread as the event loop)
        */
        std::unique_ptr<IPollableRegisterySubscriptionHandler> handler;
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

        std::size_t monitoredFdsCount = m_oRegistery.m_oMonitoredFdRegistry.GetMonitoredFDsCount();
        pollfd *monitoredFdsPtr = m_oRegistery.m_oMonitoredFdRegistry.GetMonitoredFDs();
        int iPollStatus = poll(monitoredFdsPtr, monitoredFdsCount, 0);
        if (iPollStatus == -1)
        {
            m_bRunning.store(false, std::memory_order_relaxed);
            return AsyncIO::Result{.success = false, .message = "Error in polling Descriptors"};
        }
        //! Iterate over file desc being polled
        for (unsigned int i = 0; i < monitoredFdsCount; ++i)
        {
            pollfd monitoredFd = m_oRegistery.m_oMonitoredFdRegistry.GetMonitoredFDByIndex(i);
            if (AsyncIO::PollHelpers::IsReady(monitoredFd.revents))
            {
                auto callbackIt = m_oRegistery.m_oCallbackRegistry.m_mapCallBacks.find(monitoredFd.fd);
                if (callbackIt == m_oRegistery.m_oCallbackRegistry.m_mapCallBacks.end())
                {
                    continue;
                }
                callbackIt->second(EventContext{
                    .id = monitoredFd.fd,
                    .readyEvents = monitoredFd.revents,
                });
            }
        }
        if (m_fMainTask)
        {
            m_fMainTask();
        }
        std::this_thread::yield();
    }
    return AsyncIO::Result{.success = true};
}

void AsyncIO::RunTime::SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(AsyncIO::EventContext)> &&callback)
{
    m_oRequestQueue.Push(std::make_unique<AsyncIO::CallbackSubscribeHandler>(CallbackSubscribeHandler::CallbackSubscribeHandlerContext{
        .callback = std::move(callback),
        .fd = id,
        .eventsToSubTo = eventsToSubscribeTo,
    }));
}

void AsyncIO::RunTime::UnRegisterFromAllEvents(int id)
{
    m_oRequestQueue.Push(std::make_unique<AsyncIO::CallbackUnSubscribeHandler>(CallbackUnSubscribeHandler::CallbackUnSubscribeHandlerContext{
        .fd = id,
    }));
}

void AsyncIO::RunTime::Stop()
{
    m_oRequestQueue.StopReading();
    m_oRequestQueue.StopWriting();
    m_bRunning.store(false, std::memory_order_relaxed);
}
