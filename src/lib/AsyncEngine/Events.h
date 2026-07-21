#pragma once

namespace AsyncIO
{
    enum EventType
    {
        Read = 0x0001,
        WriteSpaceAvailable = 0x0004,
        CLOSE = 0x0010,
    };
}