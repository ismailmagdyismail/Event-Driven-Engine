#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

//! Async Engine Includes
#include "PollUtils.h"
#include "TCPSocket.h"
#include "TCPServerSocket.h"
#include "RunTime.h"
#include "Events.h"
#include "AcceptFuture.h"
#include "ReadFuture.h"

AsyncIO::RunTime loop;
auto serverSocketCreation = AsyncIO::TCPServerSocket::Create(&loop);
AsyncIO::TCPServerSocket &serverSocket = serverSocketCreation.second;
std::unordered_map<unsigned int, std::unique_ptr<AsyncIO::TCPClientSocket>> activeConnections;
int recievedCount = 0;
int totalBytes = 0;

void EchoBack(AsyncIO::TCPClientSocket *socket, char *buffer, unsigned int size)
{
    socket->WriteAll(buffer, size);
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
    std::cout << "Server Listening on Port " << port << std::endl;

    char buffer[1024];
    unsigned int size = 1024;

    serverSocket
        .Accept()
        ->Then([&](std::unique_ptr<AsyncIO::TCPClientSocket> clientSocket)
               { std::cerr << "client connection made " << clientSocket->GetID()  << std::endl; return clientSocket->Read(buffer, size); })
        ->Then([](std::pair<char *, unsigned int> data)
               { std::cerr << "data recieved " << std::string_view(data.first, data.second) << std::endl; });

    std::cerr
        << "chain created " << std::endl;

    // ->Then([](char *buffer, unsigned int size)
    //    { std::cerr << "Read bytes = " << std::string_view(buffer, size) << std::endl; });
    loop.Run();

    serverSocket.Close();
}