//! System Includes
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//! Async IO
#include "TCPSocket.h"

std::pair<bool, AsyncIO::SocketInfo> AsyncIO::CreateTCPSocket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return {
            false,
            AsyncIO::SocketInfo{},
        };
    }
    return {
        true,
        AsyncIO::SocketInfo{
            .fd = fd,
        },
    };
}

bool AsyncIO::MakeNonBlocking(int fd)
{
    return fcntl(fd, F_SETFL, O_NONBLOCK) != -1;
}

sockaddr_in AsyncIO::CreateLocalAddress(unsigned int port)
{
    sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    return address;
}