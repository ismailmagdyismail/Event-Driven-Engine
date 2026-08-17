#include <iostream>
#include <unordered_map>

#include "TCPServerSocket.h"
#include "TerminalIO.h"

std::unordered_map<int, std::unique_ptr<AsyncIO::TCPClientSocket>> connections;
std::string strPrompt = "\n>> ";
std::string strConnectionAccLog = "Connection Accepted!";
std::string strDisconnectionLog = "Client Disconnected!";
std::string strRecieveLogMessage = "Recieved Message from Client: ";
std::string strUnkownOpMessage = "Unknown Server operation: ";
std::string strBroadcastMessage = "Broadcast Message from other Clients: ";
std::string strServerLaunchFailure = "Failed listen on port 9090";

std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> result;
    std::string current;

    for (char c : s)
    {
        if (c == delimiter)
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    result.push_back(current);
    return result;
}

void Broadcast(std::string_view slice, std::function<bool(AsyncIO::TCPClientSocket *)> ShouldfilterOut)
{
    //! broadcast back
    std::string strEchoMessage = strBroadcastMessage;
    strEchoMessage += slice;
    for (auto &entry : connections)
    {
        if (ShouldfilterOut(entry.second.get()))
        {
            continue;
        }
        entry.second->WriteAll(strEchoMessage.data(), strEchoMessage.size());
    }
}

int main()
{
    AsyncIO::EventLoop loop;
    AsyncIO::TerminalIO terminal(AsyncIO::TerminalIO::TerminalType::STDIN, &loop);
    auto result = AsyncIO::TCPServerSocket::Create(&loop);
    auto &serverSocket = result.second;

    auto onTerminalRead = [&](char *buffer, unsigned int size)
    {
        auto logUnknownOp = [&]()
        {
            terminal.WriteAll(strUnkownOpMessage.data(), strUnkownOpMessage.size());
            terminal.WriteAll(strPrompt.data(), strPrompt.size());
        };
        std::string operation(buffer, size);
        auto operands = split(operation, ' ');
        if (operands.size() < 1)
        {
            logUnknownOp();
            return;
        }
        if (operands.size() == 1)
        {
            operation.erase(std::remove_if(operation.begin(), operation.end(), [](unsigned char c)
                                           { return std::isspace(c); }),
                            operation.end());
            if (operation == "exit")
            {
                for (auto &entry : connections)
                {
                    entry.second->Close();
                }
                loop.Stop();
            }
            else
            {
                logUnknownOp();
                return;
            }
        }
        else if (operands.size() == 2)
        {
            auto &op1 = operands[0];
            auto &op2 = operands[1];
            if (op1 != "broadcast")
            {
                logUnknownOp();
                return;
            }
            Broadcast(op2, [](AsyncIO::TCPClientSocket *)
                      { return false; });
            terminal.WriteAll(strPrompt.data(), strPrompt.size());
        }
        else
        {
            logUnknownOp();
            return;
        }
    };
    terminal.OnRead(onTerminalRead);

    auto onReadFromClient = [&terminal](AsyncIO::TCPClientSocket *socketPtr, char *buffer, unsigned int size)
    {
        //! Log it
        std::string_view slice(buffer, size);
        std::string strClientMessage = strRecieveLogMessage;
        strClientMessage += slice;
        terminal.WriteAll(strClientMessage.data(), strClientMessage.size());
        terminal.WriteAll(strPrompt.data(), strPrompt.size());

        Broadcast(slice, [&](AsyncIO::TCPClientSocket *sock)
                  { return sock->GetID() == socketPtr->GetID(); });
    };
    auto onClientConnClose = [&]()
    {
        terminal.WriteAll(strDisconnectionLog.data(), strDisconnectionLog.size());
        terminal.WriteAll(strPrompt.data(), strPrompt.size());
    };
    auto onConnAccept = [&](std::unique_ptr<AsyncIO::TCPClientSocket> clientSocket)
    {
        //! Log Connection
        terminal.WriteAll(strConnectionAccLog.data(), strConnectionAccLog.size());
        terminal.WriteAll(strPrompt.data(), strPrompt.size());

        //! Setup callbacks
        clientSocket->OnRead(std::bind(onReadFromClient, clientSocket.get(), std::placeholders::_1, std::placeholders::_2));
        clientSocket->OnClose(onClientConnClose);

        //! Add to registry
        auto id = clientSocket->GetID();
        connections[id] = std::move(clientSocket);
    };
    serverSocket.OnAccept(onConnAccept);

    auto res = serverSocket.Listen(9090);
    if (!res.success)
    {
        terminal.WriteAll(strServerLaunchFailure.data(), strServerLaunchFailure.size());
        return -1;
    }

    terminal.WriteAll(strPrompt.data(), strPrompt.size());

    loop.Run();

    serverSocket.Close();
}