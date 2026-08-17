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
auto serverSocketCreation = AsyncIO::TCPServerSocket::Create(&loop);
AsyncIO::TCPServerSocket &serverSocket = serverSocketCreation.second;
std::unordered_map<unsigned int, std::unique_ptr<AsyncIO::TCPClientSocket>> activeConnections;
int recievedCount = 0;
int totalBytes = 0;

void EchoBack(AsyncIO::TCPClientSocket *socket, char *buffer, unsigned int size)
{
    char *bufferCpy = new char[size];
    std::memcpy(bufferCpy, buffer, size);
    socket->WriteAll(bufferCpy, size);
}

void OnClientRead(AsyncIO::TCPClientSocket *socket, char *buffer, unsigned int size)
{
    std::string_view slice(buffer, size);
    ++recievedCount;
    totalBytes += size;
    std::cerr << "read message from socket " << socket->GetID() << " message: " << slice << " with size = " << size << std::endl;
    std::cerr << "segments recieved count " << recievedCount << std::endl;
    std::cerr << "segments total bytes count " << totalBytes << std::endl;
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