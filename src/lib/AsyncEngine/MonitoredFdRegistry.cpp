//! System Includes
#include <stdexcept>

//! Async Engine Includes
#include "MonitoredFdRegistry.h"
#include "PollUtils.h"

bool AsyncIO::MonitoredFdRegistry::Contains(int fd) const
{
    return m_mapMonitoredFDsIndex.find(fd) != m_mapMonitoredFDsIndex.end();
}

pollfd *AsyncIO::MonitoredFdRegistry::GetMonitoredFDs()
{
    return m_vecMonitoredFDs.data();
}

std::size_t AsyncIO::MonitoredFdRegistry::GetMonitoredFDsCount() const
{
    return m_vecMonitoredFDs.size();
}

pollfd AsyncIO::MonitoredFdRegistry::GetMonitoredFDByIndex(std::size_t idx) const
{
    if (idx >= m_vecMonitoredFDs.size())
    {
        throw std::out_of_range("Monitored FD Index out of bounds");
    }
    return m_vecMonitoredFDs[idx];
}

void AsyncIO::MonitoredFdRegistry::AddOrUpdate(int fd, short eventsToSubTo)
{
    auto monitoredFdIt = m_mapMonitoredFDsIndex.find(fd);
    if (monitoredFdIt != m_mapMonitoredFDsIndex.end())
    {
        m_vecMonitoredFDs[monitoredFdIt->second].events =
            AsyncIO::PollHelpers::SetEvent(m_vecMonitoredFDs[monitoredFdIt->second].events, eventsToSubTo);
        return;
    }

    pollfd monitoredFd{};
    monitoredFd.fd = fd;
    monitoredFd.events = eventsToSubTo;
    m_vecMonitoredFDs.push_back(monitoredFd);
    m_mapMonitoredFDsIndex[fd] = m_vecMonitoredFDs.size() - 1;
}

void AsyncIO::MonitoredFdRegistry::Remove(int fd)
{
    auto monitoredFdIt = m_mapMonitoredFDsIndex.find(fd);
    if (monitoredFdIt == m_mapMonitoredFDsIndex.end())
    {
        return;
    }

    std::size_t idx = monitoredFdIt->second;
    std::size_t lastIdx = m_vecMonitoredFDs.size() - 1;
    if (idx != lastIdx)
    {
        m_vecMonitoredFDs[idx] = m_vecMonitoredFDs[lastIdx];
        m_mapMonitoredFDsIndex[m_vecMonitoredFDs[idx].fd] = idx;
    }

    m_vecMonitoredFDs.pop_back();
    m_mapMonitoredFDsIndex.erase(monitoredFdIt);
}
