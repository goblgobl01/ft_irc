#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"
#include "../../includes/Channel.hpp"

void    Server::handleInvite(Client &client, std::stringstream &ss)
{
    std::string targetNick;
    std::string channelName;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();

    if (!(ss >> targetNick >> channelName))
    {
        send_error(fd, "461", nick, "INVITE", "Not enough parameters");
    }
        return ;
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
    if (channel->isInviteOnly() && !channel->isOperator(&client))
    {
        send_error(fd, "482", nick, channelName, "You're not channel operator");
        return ;
    }
    Client *target = findClientByNick(targetNick);
    if (!target)
    {
        send_error(fd, "401", nick, targetNick, "No such nick/channel");
        return ;
    }
    if (channel->isMember(target))
    {
        send_error(fd, "443", nick, targetNick + " " + channelName, "is already on channel");
        return ;
    }
    channel->addInvite(target);
    sendToClient(client.get_client_fd(), ":localhost 341 " + client.get_nickname() + " " + targetNick + " " + channelName);
    sendToClient(target->get_client_fd(), getClientPrefix(client) + " INVITE " + targetNick + " " + channelName);
}