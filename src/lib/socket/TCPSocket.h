#pragma once

#include <utility>
#include <arpa/inet.h>

namespace AsyncIO
{
    struct SocketInfo
    {
        int fd;
    };

    //! This is a shallow, not so useful abstraction, but works for now
    std::pair<bool, SocketInfo> CreateTCPSocket();
    bool MakeNonBlocking(int fd);
    sockaddr_in CreateLocalAddress(unsigned int port);
}