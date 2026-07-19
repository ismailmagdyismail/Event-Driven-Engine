#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int clientFD = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = PF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    int connectionStatus = connect(clientFD, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (connectionStatus == -1)
    {
        std::cerr << "client failed to connect " << std::endl;
    }
    else
    {
        std::cout << "Client connect successfully " << std::endl;

        constexpr std::size_t size = 1024;
        char buffer[size] = "hello world";
        write(clientFD, buffer, size);
        std::cout << "Client wrote message successfully " << std::endl;

        char readBuffer[size];
        std::memset(readBuffer, 0, size);
        int readSize = read(clientFD, readBuffer, size);
        std::cout << "Recieved Message / Echo " << readSize << " == " << readBuffer << std::endl;
    }
}