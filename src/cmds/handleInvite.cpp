#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"


void    Server::handleInvite(Client &client, std::stringstream &ss)
{
    std::string targetNick;
    std::string channelName;
    std::string nick = client.get_nickname();
    int         fd   = client.get_client_fd();


    if (!(ss >> targetNick >> channelName))
    {
        send_error(fd, "461", nick, "INVITE", "Not enough parameters");
        return ;
    }

    //  Channel must exist 
    Channel *channel = findChannel(channelName);
    if (!channel)
    {
        send_error(fd, "403", nick, channelName, "No such channel");
        return ;
    }

    // Inviter must be on the channel 
    if (!channel->isMember(&client))
    {
        send_error(fd, "442", nick, channelName, "You're not on that channel");
        return ;
    }

    //  Only operators may invite on invite only channels 
    if (channel->isInviteOnly() && !channel->isOperator(&client))
    {
        send_error(fd, "482", nick, channelName, "You're not channel operator");
        return ;
    }

    //  Target must exist 
    Client *target = findClientByNick(targetNick);
    if (!target)
    {
        send_error(fd, "401", nick, targetNick, "No such nick/channel");
        return ;
    }

    //  Target must not already be on the channel 
    if (channel->isMember(target))
    {
        send_error(fd, "443", nick, targetNick + " " + channelName, "is already on channel");
        return ;
    }

    //  Add to invite list 
    if (!channel->isInvited(target))
        channel->addInvite(target);

    //  Notify inviter: RPL_INVITING 
    sendToClient(fd, ":localhost 341 " + nick + " " + targetNick + " " + channelName);

    //  Notify target
    sendToClient(target->get_client_fd(),getClientPrefix(client) + " INVITE " + targetNick + " :" + channelName);
}
