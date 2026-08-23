#pragma once

//! Async Engine Includes
#include "CallbackRegistry.h"
#include "MonitoredFdRegistry.h"

namespace AsyncIO
{
    struct EventLoopRegistery
    {
        MonitoredFdRegistry m_oMonitoredFdRegistry;
        CallbackRegistry m_oCallbackRegistry;
    };
}
