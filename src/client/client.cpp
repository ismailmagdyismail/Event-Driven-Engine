#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "EventLoop.h"
#include "TCPClientSocket.h"
#include "TerminalIO.h"
#include "Events.h"

int main()
{
    AsyncIO::EventLoop loop;
    AsyncIO::TCPClientSocket socket = AsyncIO::TCPClientSocket::Create(&loop).second;
    AsyncIO::TerminalIO terminal(AsyncIO::TerminalIO::TerminalType::STDIN, &loop);

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
        std::string_view slice(buffer, size);
        socket.Write(slice.data(), size);
    };
    terminal.OnRead(std::move(onTerminalRead));

    if (!socket.Connect(8080).success)
    {
        std::cerr << "client failed to connect " << std::endl;
        return -1;
    }

    loop.Run();
}