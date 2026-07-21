#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include "PollUtils.h"
#include "TCPSocket.h"
#include "TCPServerSocket.h"

std::vector<pollfd> polledFDs;
AsyncIO::TCPServerSocket serverSocket;
/*
    - pollfd(s) are passed by copy
    - since in some instances we are modifying the vector containing pollfd(s) entries, which may trigger re-sizing
    - since elements are not ptrs, this means old refs may be invalidated
*/

void HandleTCPServerSocketReady(pollfd readyPollFD)
{
    auto result = serverSocket.Accept();
    if (result.first.success == true)
    {
        std::cout << "Client Connection accepted " << result.second.fd << std::endl;
        polledFDs.push_back(pollfd{
            .fd = result.second.fd,
            .events = POLLIN | POLLPRI,
        });
    }
    else
    {
        std::cerr << "Failed To Accept Client Connection" << std::endl;
    }
}

void HandleClientClose(pollfd readyPollFD)
{
    std::cout << "Connection closed " << readyPollFD.fd << std::endl;
    auto it = std::find_if(polledFDs.begin(), polledFDs.end(), [&](const pollfd &polledFD)
                           { return polledFD.fd == readyPollFD.fd; });
    polledFDs.erase(it);
    close(readyPollFD.fd);
}

void HandleClientSpaceAvailable(pollfd readyPollFD)
{
    unsigned int size = 1024;
    char buffer[size];
    std::string echoMessage = "Server Echos Hello world";
    std::memset(buffer, 0, size);
    std::memcpy(buffer, echoMessage.data(), echoMessage.size());
    int writtenBytes = write(readyPollFD.fd, buffer, size);
    if (writtenBytes == -1)
    {
        std::cerr << "Socket Write buffer is full, cannot write right now" << std::endl;
        auto it = std::find_if(polledFDs.begin(), polledFDs.end(), [&](pollfd &polledFD)
                               { return polledFD.fd == readyPollFD.fd; });
        it->events = AsyncIO::PollHelpers::SetEvent(it->events, POLLOUT);
    }
    else
    {
        std::cerr << "Socket Write buffer is NOT full, full bytes written = " << writtenBytes << std::endl;
        if (writtenBytes != size)
        {
            throw std::runtime_error("[FATAL]: not all bytes got written");
        }
        auto it = std::find_if(polledFDs.begin(), polledFDs.end(), [&](pollfd &polledFD)
                               { return polledFD.fd == readyPollFD.fd; });
        it->events = AsyncIO::PollHelpers::UnSetEvent(it->events, POLLOUT);
        std::cout << "Wrote Echo buffer back to client successfully " << std::endl;
    }
}

void HandleClientDataReady(pollfd readyPollFD)
{
    std::cout << "Reading Client Data" << readyPollFD.fd << std::endl;
    unsigned int size = 1024;
    char buffer[size];
    int readBytes = read(readyPollFD.fd, buffer, size);
    if (readBytes == -1)
    {
        std::cerr << "Error In Reading Client Bytes " << readyPollFD.fd << std::endl;
    }
    else if (readBytes == 0)
    {
        std::cout << "Connection Closed by Client on reading " << std::endl;
        // HandleClientClose(readyPollFD);
        throw std::runtime_error("[FATAL]: connection closed by socket.read which is unxpected, event loop handle it with higher priority than read!");
    }
    else
    {
        std::cout << "Read bytes " << readBytes << " = " << buffer << std::endl;
        //! Try sending Echo message back to client if space is available
        //! If no space available register yourself to be notified when space is available
        HandleClientSpaceAvailable(readyPollFD);
    }
}

void HandleClientSocketReady(pollfd readyPollFD)
{
    std::cout << "Client socket has some data ready " << readyPollFD.fd << std::endl;
    if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.revents, POLLHUP))
    {
        HandleClientClose(readyPollFD);
    }
    else if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.revents, POLLIN))
    {
        HandleClientDataReady(readyPollFD);
    }
    else if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.revents, POLLOUT))
    {
        HandleClientSpaceAvailable(readyPollFD);
    }
}

void HandleReadyFD(pollfd readyPollFD)
{
    if (readyPollFD.fd == serverSocket.GetSocketInfo().fd)
    {
        std::cout << "Server Desc ready " << std::endl;
        HandleTCPServerSocketReady(readyPollFD);
    }
    else
    {
        std::cout << "Client Desc ready " << std::endl;
        HandleClientSocketReady(readyPollFD);
    }
}

int main()
{
    serverSocket = AsyncIO::TCPServerSocket::Create().second;
    serverSocket.Listen(8080);

    polledFDs.push_back(pollfd{
        .fd = serverSocket.GetSocketInfo().fd,
        .events = POLLIN | POLLPRI,
    });

    while (true)
    {
        int iPollStatus = poll(polledFDs.data(), polledFDs.size(), 0);
        if (iPollStatus == -1)
        {
            std::cerr << "Error in polling Descriptors " << std::endl;
            exit(1);
        }
        //! Iterate over file desc being polled
        for (unsigned int i = 0; i < polledFDs.size(); ++i)
        {
            if (AsyncIO::PollHelpers::IsReady(polledFDs[i].revents))
            {
                HandleReadyFD(polledFDs[i]);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    close(serverSocket.GetSocketInfo().fd);
}