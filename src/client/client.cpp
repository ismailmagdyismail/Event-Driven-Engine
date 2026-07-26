#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "EventLoop.h"
#include "TCPClientSocket.h"

int main()
{
    AsyncIO::TCPClientSocket socket = AsyncIO::TCPClientSocket::Create().second;
    
    if (!socket.Connect(8080).success)
    {
        std::cerr << "client failed to connect " << std::endl;
    }
    else
    {
        std::cout << "Client connect successfully " << std::endl;

        constexpr std::size_t size = 1024;
        char buffer[size] = "hello world";
        write(socket.GetID(), buffer, size);
        std::cout << "Client wrote message successfully " << std::endl;

        char readBuffer[size];
        std::memset(readBuffer, 0, size);
        int readSize = read(socket.GetID(), readBuffer, size);
        std::cout << "Recieved Message / Echo " << readSize << " == " << readBuffer << std::endl;
    }
}