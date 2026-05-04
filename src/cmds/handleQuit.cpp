#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void	Server::removeClientFromAllChannels(Client &client, const std::string &reason)
{
    for (size_t i = 0; i < channel_vector.size();)
    {
        if (channel_vector[i].isMember(&client))
        {
            if (channel_vector[i].memberCount() > 1)
                channel_vector[i].broadcastMessage(
                    getClientPrefix(client) + " QUIT :" + reason, &client);

            channel_vector[i].removeMember(&client);
            if (channel_vector[i].memberCount() == 0)
            {
                channel_vector.erase(channel_vector.begin() + i);
                continue;
            }
        }
        i++;
    }
}

void	Server::handleQuit(Client &client, std::stringstream &ss)
{
    std::string reason = "Quit";
    std::string rest;
    if (std::getline(ss, rest))
    {
        size_t colon = rest.find(':');
        if (colon != std::string::npos)
            reason = rest.substr(colon + 1);
        else
        {
            size_t first = rest.find_first_not_of(" ");
            if (first != std::string::npos)
                reason = rest.substr(first);
        }
    }
    removeClientFromAllChannels(client, reason);
    remove_a_client(client);
}
