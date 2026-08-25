#include <iostream>
#include <unordered_map>
#include <atomic>
#include <set>
#include <unistd.h>
#include <fcntl.h>

#include "TCPServerSocket.h"
#include "TerminalIO.h"

const int SIZE = 1024 * 12;
struct ClientSession
{
    AsyncIO::TCPClientSocket *m_oConnection{nullptr};
    unsigned long long m_ullOffset{0};
    bool m_bClosed{false};
    char m_buffer[SIZE];
    bool m_bInBackPressure = false;
};

unsigned long long connectionsID = 1;
auto connectionsRegistery = std::unordered_map<int, ClientSession>();
std::string strConnectionAccLog = "Connection Accepted!";
std::string strDisconnectionLog = "Client Disconnected!\n";
std::string strServerLaunchFailure = "Failed listen on port 9090\n";

AsyncIO::RunTime loop;
AsyncIO::TerminalIO terminal(&loop);
int fileFD;
unsigned long long ullTotalFileBytes = 0;

void CreateDummyFile()
{
    fileFD = open("dummy_file.txt", O_CREAT | O_RDWR | O_TRUNC, 0755);
    unsigned long long maxFileSize = 1024 * 1024 * 10;
    unsigned long long entries = 1;
    std::string strBaseMessage = "entry_";
    while (ullTotalFileBytes < maxFileSize)
    {
        std::string strEntry = strBaseMessage + std::to_string(entries) + "\n";
        write(fileFD, strEntry.data(), strEntry.size());
        entries++;
        ullTotalFileBytes += strEntry.size();
    }
    std::string message = "total entries in dummy file " + std::to_string(entries);
    std::string message2 = "total file size " + std::to_string(ullTotalFileBytes);
    std::string newLine = "\n";
    terminal.WriteAll(message.data(), message.size());
    terminal.WriteAll(newLine.data(), newLine.size());
    terminal.WriteAll(message2.data(), message2.size());
    terminal.WriteAll(newLine.data(), newLine.size());
}

int main()
{
    auto result = AsyncIO::TCPServerSocket::Create(&loop);
    auto &serverSocket = result.second;

    CreateDummyFile();

    auto onClientConnClose = [&]()
    {
        terminal.WriteAll(strDisconnectionLog.data(), strDisconnectionLog.size());
    };
    auto onConnAccept = [&](std::unique_ptr<AsyncIO::TCPClientSocket> clientSocket)
    {
        //! Setup callbacks
        clientSocket->OnClose(onClientConnClose);

        auto fd = clientSocket->GetID();

        connectionsRegistery.insert({connectionsID, ClientSession{
                                                        .m_oConnection = clientSocket.release(),
                                                        .m_ullOffset = 0,
                                                        .m_bClosed = false,
                                                        .m_bInBackPressure = false,
                                                    }});
        //! Log Connection
        std::string log = strConnectionAccLog + " with connection " + std::to_string(connectionsID) + " and FD " + std::to_string(fd) + "\n";
        terminal.WriteAll(log.data(), log.size());
        ++connectionsID;
    };
    serverSocket.OnAccept(onConnAccept);

    auto res = serverSocket.Listen(9090);
    if (!res.success)
    {
        terminal.WriteAll(strServerLaunchFailure.data(), strServerLaunchFailure.size());
        return -1;
    }

    unsigned long long totalBytes = 0;
    unsigned long long iterations = 0;

    auto distributeFileTask = [&]()
    {
        for (auto &entry : connectionsRegistery)
        {
            auto &session = entry.second;

            //! Skip
            if (session.m_ullOffset >= ullTotalFileBytes)
            {
                if (!session.m_bClosed)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    session.m_oConnection->Close();
                    session.m_bClosed = true;
                }
                continue;
            }
            //! TEMP work around to avoid reading into session buffer while it still being consumed by soccket (overwriting bytes)
            //! could simply be done by giving sokcet its own buffer copy
            if (session.m_bInBackPressure)
            {
                continue;
            }
            int readBytes = pread(fileFD, session.m_buffer, SIZE, session.m_ullOffset);

            session.m_bInBackPressure = true;
            if (readBytes > 0 && session.m_oConnection->WriteAll(session.m_buffer, readBytes, [&]()
                                                                 { session.m_bInBackPressure = false; }))
            {
                totalBytes += readBytes;
                session.m_ullOffset += readBytes;
            }
            ++iterations;
        }
    };

    loop.AddMainTask(std::move(distributeFileTask));
    loop.Run();
    serverSocket.Close();
}