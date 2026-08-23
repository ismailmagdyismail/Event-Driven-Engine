#pragma once

//! Async Engine Includes
#include "CallbackRegistry.h"
#include "MonitoredFdRegistry.h"
#include "FutureRegistry.h"

namespace AsyncIO
{
    struct PollableRegistery
    {
        MonitoredFdRegistry m_oMonitoredFdRegistry;
        CallbackRegistry m_oCallbackRegistry;
        FutureRegistry m_oFutureRegistry;
    };
}
