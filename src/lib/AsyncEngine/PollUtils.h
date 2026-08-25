#pragma once

#include <vector>

namespace AsyncIO::PollHelpers
{
    bool IsReady(short state);
    bool IsEventSet(short state, short eventToCheck);
    short SetEvent(short state, short eventToSet);
    short UnSetEvent(short state, short eventToUnset);
    void LogEventsState(short state);
    std::vector<short> GetActiveEvents(short revents);
}
