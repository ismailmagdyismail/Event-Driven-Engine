#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "RunTime.h"
#include "TCPClientSocket.h"
#include "TerminalIO.h"
#include "Events.h"

int main()
{
    AsyncIO::RunTime loop;
    auto socketCreation = AsyncIO::TCPClientSocket::Create(&loop);
    AsyncIO::TCPClientSocket &socket = socketCreation.second;
    AsyncIO::TerminalIO terminal(&loop);

    auto onSocketRead = [&](char *buffer, unsigned int size)
    {
        std::string_view slice(buffer, size);
        std::cout << "Recieved message from server " << slice << std::endl;
    };
    auto onSocketDisconnect = [&]()
    {
        std::cout << "Server connection lost, terminating client ...." << std::endl;
        loop.Stop();
    };
    socket.OnRead(std::move(onSocketRead));
    socket.OnClose(std::move(onSocketDisconnect));

    auto onTerminalRead = [&](char *buffer, unsigned int size)
    {
        socket.WriteAll(buffer, size);
        // std::string largeBuffer(1024 * 10000, 'x');
        // std::cerr << "sent size " << largeBuffer.size() << std::endl;
    };
    terminal.OnRead(std::move(onTerminalRead));

    if (!socket.Connect(8080).success)
    {
        std::cerr << "client failed to connect " << std::endl;
        return -1;
    }

    loop.Run();
}