#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    int serverSocketFD = socket(PF_INET, SOCK_STREAM, 0);

    if (serverSocketFD == -1)
    {
        std::cerr << "Error Creating Server Socket" << std::endl;
        exit(1);
    }

    sockaddr_in address;
    address.sin_family = PF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;
    int bindStatus = bind(serverSocketFD, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (bindStatus != 0)
    {
        std::cerr << "Error Binding Server Socket" << std::endl;
        exit(1);
    }
    int iListenStatus = listen(serverSocketFD, 0);
    if (iListenStatus != 0)
    {
        std::cerr << "Error Listening on Server Socket" << std::endl;
        exit(1);
    }
    socklen_t addSize = sizeof(address);
    unsigned int size = 1024;
    char buffer[size];
    while (true)
    {
        std::cout << "Server waiting to accept connections " << std::endl;
        int clientFD = accept(serverSocketFD, reinterpret_cast<sockaddr *>(&address), &addSize);
        if (clientFD != -1)
        {
            std::cout << "Client Connection accepted " << clientFD << std::endl;
            int bytes = read(clientFD, buffer, size);
            if (bytes == 0)
            {
                std::cout << "Client Connection closed " << clientFD << std::endl;
                close(clientFD);
            }
            else
            {
                std::cout << "Echo response sent " << std::endl;
                write(clientFD, buffer, size);
            }
        }
        else
        {
            std::cerr << "Failed to accept client connection " << std::endl;
        }
    }
    close(serverSocketFD);
}