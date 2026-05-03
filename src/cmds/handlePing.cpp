#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"

void Server::handlePing(Client &client, std::stringstream &ss)
{
    std::string token;
    if (!(ss >> token))
    {
        send_error(client.get_client_fd(), "409", client.get_nickname(), "PING", "No origin specified");
        return ;
    }
    std::string response = "PONG localhost :" + token;
    sendToClient(client.get_client_fd(), response);
}
