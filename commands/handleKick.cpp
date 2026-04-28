#include "../Server.hpp"
#include "../Client.hpp"
#include "../Channel.hpp"

void    Server::handleKick(Client &client, std::stringstream &ss)
{
    std::string channelName;
    std::string targetNick;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();
    if (!(ss >> channelName >> targetNick))
    {
        send_error(fd, "461", nick, "KICK", "Not enough parameters");
        return ;
    }
    std::string reason;
    std::getline(ss, reason);
    size_t colon = reason.find(':');
    if (colon != std::string::npos)
        reason = reason.substr(colon + 1);
    else
    {
        size_t first = reason.find_first_not_of(" ");
        if (first != std::string::npos)
            reason = reason.substr(first);
        else
            reason = client.get_nickname();
    }
    Channel *channel = findChannel(channelName);
    if (!channel)
    {
        send_error(fd, "403", nick, channelName, "No such channel");
        return ;
    }
    if (!channel->isMember(&client))
    {
        send_error(fd, "442", nick, channelName, "You're not on that channel");
        return ;
    }
    if (!channel->isOperator(&client))
    {
        send_error(fd, "482", nick, channelName, "You're not channel operator");
        return ;
    }
    
    Client *target = findClientByNick(targetNick);
    
    if (!target || !channel->isMember(target))
    {
        send_error(fd, "441", nick, targetNick + " " + channelName, "They aren't on that channel");
        return ;
    }
    
    channel->broadcastMessage(getClientPrefix(client) + " KICK " + channelName + " " + targetNick + " :" + reason, NULL);
    channel->removeMember(target);
    
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
