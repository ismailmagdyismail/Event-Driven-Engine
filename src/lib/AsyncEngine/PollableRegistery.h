#pragma once

//! Async Engine Includes
#include "CallbackRegistry.h"
#include "MonitoredFdRegistry.h"

namespace AsyncIO
{
    struct PollableRegistery
    {
        MonitoredFdRegistry m_oMonitoredFdRegistry;
        CallbackRegistry m_oCallbackRegistry;
    };
}
