#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void Server::handlePing(Client &client, std::stringstream &ss)
{
    std::string token;
    std::string response = "PONG localhost :";
    if (ss >> token)
        response += token;
    sendToClient(client.get_client_fd(), response);
}
