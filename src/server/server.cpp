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

AsyncIO::EventLoop loop;
AsyncIO::TCPServerSocket serverSocket = AsyncIO::TCPServerSocket::Create(&loop).second;
std::unordered_map<unsigned int, std::unique_ptr<AsyncIO::TCPClientSocket>> activeConnections;

void EchoBack(AsyncIO::TCPClientSocket *socket, char *buffer, unsigned int size)
{
    socket->Write(buffer, size);
}

void OnClientRead(AsyncIO::TCPClientSocket *socket, char *buffer, unsigned int size)
{
    std::cerr << "read message from socket " << socket->GetID() << " with size = " << size << std::endl;
    EchoBack(socket, buffer, size);
}

void OnClientClose(AsyncIO::TCPClientSocket *clientSocket)
{
    std::cerr << "client disconnected " << std::endl;
    clientSocket->Close();
    activeConnections.erase(clientSocket->GetID());
}

void OnAcceptConnectionHandler(std::unique_ptr<AsyncIO::TCPClientSocket> clientSocket)
{
    std::cerr << "accepted connection " << std::endl;
    clientSocket->OnRead(std::bind(OnClientRead, clientSocket.get(), std::placeholders::_1, std::placeholders::_2));
    clientSocket->OnClose(std::bind(OnClientClose, clientSocket.get()));
    activeConnections[clientSocket->GetID()] = std::move(clientSocket);
}

int main()
{
    int port = 8080;
    auto listenResult = serverSocket.Listen(port);
    if (!listenResult.success)
    {
        std::cerr << "Error in listening to port " << port << std::endl;
        std::cerr << listenResult.message << std::endl;
        exit(1);
    }

    serverSocket.OnAccept(OnAcceptConnectionHandler);
    loop.Run();

    serverSocket.Close();
}