#pragma once

//! System Includes
#include <cstddef>
#include <poll.h>
#include <unordered_map>
#include <vector>

namespace AsyncIO
{
    class MonitoredFdRegistry
    {
    public:
        bool Contains(int fd) const;
        void AddOrUpdate(int fd, short eventsToSubTo);
        void Remove(int fd);
        pollfd GetMonitoredFDByIndex(std::size_t idx) const;
        pollfd *GetMonitoredFDs();
        std::size_t GetMonitoredFDsCount() const;

    private:
        //! A Page Table / Buffer Pool inspired design
        //! Index + actual data
        std::unordered_map<int, std::size_t> m_mapMonitoredFDsIndex;
        std::vector<pollfd> m_vecMonitoredFDs;
    };
}
