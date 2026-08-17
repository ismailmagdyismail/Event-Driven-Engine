#include <iostream>
#include <string>

#include "TCPClientSocket.h"
#include "TerminalIO.h"

std::string strPrompt(">> ");

int main()
{
    AsyncIO::EventLoop loop;
    AsyncIO::TerminalIO terminal(AsyncIO::TerminalIO::TerminalType::STDIN, &loop);
    auto res = AsyncIO::TCPClientSocket::Create(&loop);
    AsyncIO::TCPClientSocket &socket = res.second;
    auto ConnRes = socket.Connect(9090);
    if (!ConnRes.success)
    {
        std::cerr << "Connection failed " << std::endl;
        terminal.WriteAll(ConnRes.message.data(), ConnRes.message.size());
        return -1;
    }

    terminal.OnRead([&](char *buffer, unsigned int size)
                    {
        socket.WriteAll(buffer, size);
        terminal.WriteAll(strPrompt.data(), strPrompt.size()); });

    socket.OnRead([&](char *buffer, unsigned int size)
                  { 
                    std::string_view slice (buffer,size);
                    std::string strMessage = "Message Recieved: " ;
                    strMessage += slice;
                    terminal.WriteAll(strMessage.data(), strMessage.size());
                    terminal.WriteAll(strPrompt.data(), strPrompt.size()); });

    socket.OnClose([&]()
                   {
                    std::string strDisconnectMessage = "Server connection lost, terminating client ....";
                    terminal.WriteAll(strDisconnectMessage.data(), strDisconnectMessage.size()); 
                    loop.Stop(); });

    //! Inital prompt
    terminal.WriteAll(strPrompt.data(), strPrompt.size());

    loop.Run();
}