#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

//! Check any bit is not set to 0
//! thus any some type of event is ready
inline bool IsReady(short state)
{
    return state != 0;
}

//! Check for specific event being set

inline bool IsEventSet(short state, short eventToCheck)
{
    return (state & eventToCheck) != 0;
}

inline void SetEvent(short &state, short eventToSet)
{
    state |= eventToSet;
}

inline void UnSetEvent(short &state, short eventToUnset)
{
    state &= ~eventToUnset;
}

inline void LogEventsState(short state)
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

int serverSocketFD{-1};
sockaddr_in serverAddress;
socklen_t addressSize;
std::vector<pollfd> polledFDs;

/*
    - pollfd(s) are passed by copy
    - since in some instances we are modifying the vector containing pollfd(s) entries, which may trigger re-sizing
    - since elements are not ptrs, this means old refs may be invalidated
*/

void HandleServerSocketReady(pollfd readyPollFD)
{
    int clientFD = accept(readyPollFD.fd, reinterpret_cast<sockaddr *>(&serverAddress), &addressSize);
    if (clientFD != -1)
    {
        std::cout << "Client Connection accepted " << clientFD << std::endl;
        fcntl(serverSocketFD, F_SETFL, O_NONBLOCK);
        polledFDs.push_back(pollfd{
            .fd = clientFD,
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
        SetEvent(it->events, POLLOUT);
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
        UnSetEvent(it->events, POLLOUT);
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
    if (IsEventSet(readyPollFD.revents, POLLHUP))
    {
        HandleClientClose(readyPollFD);
    }
    else if (IsEventSet(readyPollFD.revents, POLLIN))
    {
        HandleClientDataReady(readyPollFD);
    }
    else if (IsEventSet(readyPollFD.revents, POLLOUT))
    {
        HandleClientSpaceAvailable(readyPollFD);
    }
}

void HandleReadyFD(pollfd readyPollFD)
{
    if (readyPollFD.fd == serverSocketFD)
    {
        std::cout << "Server Desc ready " << std::endl;
        HandleServerSocketReady(readyPollFD);
    }
    else
    {
        std::cout << "Client Desc ready " << std::endl;
        HandleClientSocketReady(readyPollFD);
    }
}

int main()
{
    serverSocketFD = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocketFD == -1)
    {
        std::cerr << "Error Creating Server Socket" << std::endl;
        exit(1);
    }
    fcntl(serverSocketFD, F_SETFL, O_NONBLOCK);
    serverAddress.sin_family = PF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    addressSize = sizeof(serverAddress);
    int bindStatus = bind(serverSocketFD, reinterpret_cast<sockaddr *>(&serverAddress), addressSize);
    if (bindStatus != 0)
    {
        std::cerr << "Error Binding Server Socket" << std::endl;
        exit(1);
    }
    int iListenStatus = listen(serverSocketFD, 0);
    if (iListenStatus != 0)
    {
        std::cerr << "Error Listening on Server Socket" << std::endl;
        exit(1);
    }
    polledFDs.push_back(pollfd{
        .fd = serverSocketFD,
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
            if (IsReady(polledFDs[i].revents))
            {
                HandleReadyFD(polledFDs[i]);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    close(serverSocketFD);
}