#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include "TCPClientSocket.h"
#include "TerminalIO.h"

unsigned long long ullTotalBytes = 0;
int main()
{
    AsyncIO::RunTime loop;
    AsyncIO::TerminalIO terminal(&loop);
    auto res = AsyncIO::TCPClientSocket::Create(&loop);
    AsyncIO::TCPClientSocket &socket = res.second;

    auto ConnRes = socket.Connect(9090);
    if (!ConnRes.success)
    {
        terminal.WriteAll(ConnRes.message.data(), ConnRes.message.size());
        return -1;
    }

    std::string connectionMessage = "Connected to server successfully\n";
    terminal.WriteAll(connectionMessage.data(), connectionMessage.size());
    auto pid = getpid();
    std::string fileName = "assembled_file_" + std::to_string(pid) + ".txt";
    int fd = open(fileName.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0755);

    auto onRead = [&](char *buffer, unsigned int size)
    {
        // std::string strMessage = "Chunk of size " + std::to_string(size) + " Recieved\n";
        // terminal.WriteAll(strMessage.data(), strMessage.size());
        ullTotalBytes += size;
        write(fd, buffer, size);
        fsync(fd);
    };
    auto onClose = [&]()
    {
        std::string strDisconnectMessage = "Server connection lost, terminating client ....";
        terminal.WriteAll(strDisconnectMessage.data(), strDisconnectMessage.size());
        loop.Stop();
    };

    socket.OnRead(onRead);
    socket.OnClose(onClose);

    loop.Run();

    std::string strLog = "Total recieved bytes " + std::to_string(ullTotalBytes) + "\n";
    terminal.WriteAll(strLog.data(), strLog.size());
}