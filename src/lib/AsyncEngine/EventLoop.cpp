//! System Includes
#include <poll.h>
#include <iostream>
#include <thread>
#include <chrono>

//! Async Engine Includes
#include "EventLoop.h"
#include "PollUtils.h"

AsyncIO::EventLoop::EventLoop()
{
    m_bRunning.store(false, std::memory_order_relaxed);
}

AsyncIO::Result AsyncIO::EventLoop::Run()
{
    m_bRunning.store(true, std::memory_order_relaxed);
    while (m_bRunning.load(std::memory_order_relaxed))
    {
        int iPollStatus = poll(m_vecMonitoredFDs.data(), m_vecMonitoredFDs.size(), 0);
        if (iPollStatus == -1)
        {
            m_bRunning.store(false, std::memory_order_relaxed);
            return AsyncIO::Result{.success = false, .message = "Error in polling Descriptors"};
        }
        //! Iterate over file desc being polled
        for (unsigned int i = 0; i < m_vecMonitoredFDs.size(); ++i)
        {
            if (AsyncIO::PollHelpers::IsReady(m_vecMonitoredFDs[i].revents))
            {
                // HandleReadyFD(polledFDs[i]);
                m_mapCallBacks[m_vecMonitoredFDs[i].fd](EventContext{
                    .id = m_vecMonitoredFDs[i].fd,
                    .readyEvents = m_vecMonitoredFDs[i].revents,
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return AsyncIO::Result{.success = true};
}

void AsyncIO::EventLoop::SubScribeToEvent(int id, short eventsToSubscribeTo, std::function<void(AsyncIO::EventContext)> &&callback)
{
    //! TODO: look up hash map first to make sure no double entries exist
    //! TODO: RCU | COW
    m_vecMonitoredFDs.push_back(pollfd{
        .fd = id,
        .events = eventsToSubscribeTo,
    });
    m_mapCallBacks[id] = std::move(callback);
}

void AsyncIO::EventLoop::UnRegisterFromAllEvents(int id)
{
    //! TODO: hashup
    auto it = std::find_if(m_vecMonitoredFDs.begin(), m_vecMonitoredFDs.end(), [&](const pollfd &polledFD)
                           { return polledFD.fd == id; });
    m_vecMonitoredFDs.erase(it);
    m_mapCallBacks.erase(it->fd);
}

void AsyncIO::EventLoop::Stop()
{
    m_bRunning.store(false, std::memory_order_relaxed);
}