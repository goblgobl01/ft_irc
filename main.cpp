#include "irc.hpp"
#define PORT 5000

// int getaddrinfo(const char *node,   // e.g. "www.example.com" or IP
//             const char *service,  // e.g. "http" or port number
//             const struct addrinfo *hints, // hints contain the thing that i need it will be filled with the goodies
//             struct addrinfo **res);
// std::cout << "Hello World!" << std::endl;

int main()
{
    // creating my own server that listen on port 50000
    // Create a socket
    int my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (my_socket == -1)
    {
        std::cerr << errno << std::endl;
        return (-1);
    }
    // bind the socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(50000);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
    if (bind(my_socket, (struct sockaddr *)&hint, sizeof(hint)) == -1)
    {
        std::cerr << errno << std::endl;
        return (-1);
    }
    // Mark the socket for listening
    if (listen(my_socket, SOMAXCONN) == -1)
    {
        std::cerr << errno << std::endl;
        return (-1);
    }
    // Accept a call
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    int clientSocket = accept(my_socket, (struct sockaddr *)&client, &clientSize);
    if (clientSocket == -1) {
        std::cerr << "Problem with client connecting!" << std::endl;
        return -1;
    }
    // the loop
    char buffer[4096];
    memset(buffer, 0, 4096);

    while(true)
    {
        int bytesReceived = recv(clientSocket, buffer, 4096, 0);
        if (bytesReceived == -1) {
            std::cerr << "Error in recv(). Quitting" << std::endl;
            break;
        }
        if (bytesReceived == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        std::cout << "Received: " << std::string(buffer, 0, bytesReceived) << std::endl;
    }
    close(clientSocket);
    close(my_socket);
    return (0);
    // close the listening socket
    // while receiving display echo message
    // close socket
}