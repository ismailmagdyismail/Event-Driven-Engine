#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include "PollUtils.h"
#include "TCPSocket.h"
#include "TCPServerSocket.h"
#include "EventLoop.h"
#include "Events.h"

AsyncIO::TCPServerSocket serverSocket;
AsyncIO::EventLoop loop;

/*
    - AsyncIO::EventContext(s) are passed by copy
    - since in some instances we are modifying the vector containing AsyncIO::EventContext(s) entries, which may trigger re-sizing
    - since elements are not ptrs, this means old refs may be invalidated
*/

void HandleClientDataReady(AsyncIO::EventContext readyPollFD);
void HandleClientSocketReady(AsyncIO::EventContext readyPollFD);

void HandleTCPServerSocketReady(AsyncIO::EventContext readyPollFD)
{
    auto result = serverSocket.Accept();
    if (result.first.success == true)
    {
        std::cout << "Client Connection accepted " << result.second.fd << std::endl;
        loop.SubScribeToEvent(result.second.fd, AsyncIO::EventType::Read, [](AsyncIO::EventContext ctx)
                              { HandleClientSocketReady(ctx); });
    }
    else
    {
        std::cerr << "Failed To Accept Client Connection" << std::endl;
    }
}

void HandleClientClose(AsyncIO::EventContext readyPollFD)
{
    std::cout << "Connection closed " << readyPollFD.id << std::endl;
    loop.UnRegisterFromAllEvents(readyPollFD.id);
    // close(readyPollFD.fd);
}

void HandleClientSpaceAvailable(AsyncIO::EventContext readyPollFD)
{
    unsigned int size = 1024;
    char buffer[size];
    std::string echoMessage = "Server Echos Hello world";
    std::memset(buffer, 0, size);
    std::memcpy(buffer, echoMessage.data(), echoMessage.size());
    int writtenBytes = write(readyPollFD.id, buffer, size);
    if (writtenBytes == -1)
    {
        std::cerr << "Socket Write buffer is full, cannot write right now" << std::endl;
    }
    else
    {
        std::cerr << "Socket Write buffer is NOT full, full bytes written = " << writtenBytes << std::endl;
        std::cout << "Wrote Echo buffer back to client successfully " << std::endl;

        if (writtenBytes != size)
        {
            throw std::runtime_error("[FATAL]: not all bytes got written");
        }
    }

    //! TODO: chunked writes
    // auto it = std::find_if(polledFDs.begin(), polledFDs.end(), [&](AsyncIO::EventContext &polledFD)
    //    { return polledFD.fd == readyPollFD.fd; });
    // 1[Subscribe to when space is available]it->events = AsyncIO::PollHelpers::SetEvent(it->events, POLLOUT);
    // 2[un-subscribe since all message is written].it->events = AsyncIO::PollHelpers::UnSetEvent(it->events, POLLOUT);
}

void HandleClientDataReady(AsyncIO::EventContext readyPollFD)
{
    std::cout << "Reading Client Data" << readyPollFD.id << std::endl;
    unsigned int size = 1024;
    char buffer[size];
    int readBytes = read(readyPollFD.id, buffer, size);
    if (readBytes == -1)
    {
        std::cerr << "Error In Reading Client Bytes " << readyPollFD.id << std::endl;
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

void HandleClientSocketReady(AsyncIO::EventContext readyPollFD)
{
    std::cout << "Client socket has some data ready " << readyPollFD.id << std::endl;
    if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.readyEvents, POLLHUP))
    {
        HandleClientClose(readyPollFD);
    }
    else if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.readyEvents, POLLIN))
    {
        HandleClientDataReady(readyPollFD);
    }
    else if (AsyncIO::PollHelpers::IsEventSet(readyPollFD.readyEvents, POLLOUT))
    {
        HandleClientSpaceAvailable(readyPollFD);
    }
}

void HandleReadyFD(AsyncIO::EventContext readyPollFD)
{
    if (readyPollFD.id == serverSocket.GetID())
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

    loop.SubScribeToEvent(serverSocket.GetID(), AsyncIO::EventType::Read, [](AsyncIO::EventContext context)
                          { HandleReadyFD(context); });
    loop.Run();

    close(serverSocket.GetID());
}