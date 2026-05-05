#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

/*
    KICK <channel> <user> [:<reason>]

    Ejects a user from a channel. Only channel operators may use KICK.

Behaviour:
   - The kick message is broadcast to ALL members (including the target)
        BEFORE the target is removed, so they receive the notification.
   - If the target was an operator, they are removed from the ops list too.
   - If the target was on the invite list the invite is revoked so they
        cannot silently rejoin an invite only channel.
   - If the channel becomes empty after the kick, it is destroyed.
   - Reason defaults to the kicker's nickname if none is supplied.

*/

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
        // Standard IRC trailing param: everything after ':'
        reason = reason.substr(colon + 1);
    }
    else
    {
        // No colon: trim leading spaces
        size_t first = reason.find_first_not_of(" \t");
        reason = (first != std::string::npos) ? reason.substr(first) : "";
    }

    // Default reason is kicker's nickname (common IRC convention)
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

    // --- Clean up: remove from operators list if applicable 
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
