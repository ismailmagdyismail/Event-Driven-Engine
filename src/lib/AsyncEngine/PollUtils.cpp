//! System Includes
#include <poll.h>
#include <iostream>

//! Async IO Includes
#include "PollUtils.h"

bool AsyncIO::PollHelpers::IsReady(short state)
{
    return state != 0;
}

bool AsyncIO::PollHelpers::IsEventSet(short state, short eventToCheck)
{
    return (state & eventToCheck) != 0;
}

short AsyncIO::PollHelpers::SetEvent(short state, short eventToSet)
{
    return state |= eventToSet;
}

short AsyncIO::PollHelpers::UnSetEvent(short state, short eventToUnset)
{
    return state &= ~eventToUnset;
}

void AsyncIO::PollHelpers::LogEventsState(short state)
{
    std::cout << "POLLIN = " << IsEventSet(state, POLLIN) << std::endl;
    std::cout << "POLLOUT = " << IsEventSet(state, POLLOUT) << std::endl;
    std::cout << "POLLRDNORM = " << IsEventSet(state, POLLRDNORM) << std::endl;
    std::cout << "POLLWRNORM = " << IsEventSet(state, POLLWRNORM) << std::endl;
    std::cout << "POLLRDBAND = " << IsEventSet(state, POLLRDBAND) << std::endl;
    std::cout << "POLLWRBAND = " << IsEventSet(state, POLLWRBAND) << std::endl;
    std::cout << "POLLEXTEND = " << IsEventSet(state, POLLEXTEND) << std::endl;
    std::cout << "POLLATTRIB = " << IsEventSet(state, POLLATTRIB) << std::endl;
    std::cout << "POLLNLINK = " << IsEventSet(state, POLLNLINK) << std::endl;
    std::cout << "POLLERR = " << IsEventSet(state, POLLERR) << std::endl;
    std::cout << "POLLHUP = " << IsEventSet(state, POLLHUP) << std::endl;
    std::cout << "POLLNVAL = " << IsEventSet(state, POLLNVAL) << std::endl;
}

std::vector<short> AsyncIO::PollHelpers::GetActiveEvents(short revents)
{
    std::vector<short> activeEvents;
    for (short event = 1; event != 0; event <<= 1)
    {
        if (AsyncIO::PollHelpers::IsEventSet(revents, event))
        {
            activeEvents.push_back(event);
        }
    }

    return activeEvents;
}