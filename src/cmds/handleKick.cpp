#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

// KICK <channel> <user> [:<reason>]


void    Server::handleKick(Client &client, std::stringstream &ss)
{
    std::string channelName;
    std::string targetNick;
    std::string nick = client.get_nickname();
    int         fd   = client.get_client_fd();


    if (!(ss >> channelName >> targetNick))
    {
        send_error(fd, "461", nick, "KICK", "Not enough parameters");
        return ;
    }


    std::string reason;
    std::getline(ss, reason);

    size_t colon = reason.find(':');
    if (colon != std::string::npos)
    {
        reason = reason.substr(colon + 1);
    }
    else
    {
        size_t first = reason.find_first_not_of(" \t");
        reason = (first != std::string::npos) ? reason.substr(first) : "";
    }


    if (reason.empty())
        reason = nick;

    // Channel must exist 
    Channel *channel = findChannel(channelName);
    if (!channel)
    {
        send_error(fd, "403", nick, channelName, "No such channel");
        return ;
    }

    //  Kicker must be on the channel 
    if (!channel->isMember(&client))
    {
        send_error(fd, "442", nick, channelName, "You're not on that channel");
        return ;
    }

    //  Kicker must be an operator 
    if (!channel->isOperator(&client))
    {
        send_error(fd, "482", nick, channelName, "You're not channel operator");
        return ;
    }

    //  Target must exist and be on the channel 
    Client *target = findClientByNick(targetNick);
    if (!target || !channel->isMember(target))
    {
        send_error(fd, "441", nick, targetNick + " " + channelName, "They aren't on that channel");
        return ;
    }

    //  Broadcast KICK to everyone (including target) BEFORE removal 
    channel->broadcastMessage(getClientPrefix(client) + " KICK " + channelName + " " + targetNick + " :" + reason, NULL);

    // remove from operators list if applicable 
    if (channel->isOperator(target))
        channel->removeOperator(target);

    // Clean up: revoke any pending invite so they can't silently rejoin 
    if (channel->isInvited(target))
        channel->removeInvite(target);

    //  Remove from channel
    channel->removeMember(target);

    //  Destroy empty channel 
    if (channel->memberCount() == 0)
    {
        for (size_t i = 0; i < channel_vector.size(); i++)
        {
            if (channel_vector[i].getName() == channelName)
            {
                channel_vector.erase(channel_vector.begin() + i);
                break ;
            }
        }
    }
}
