#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"

void    Server::announceJoin(Client &client, Channel *channel, const std::string &channelName)
{
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();

    channel->broadcastMessage(getClientPrefix(client) + " JOIN " + channelName, NULL); //  broadcast join to all channel members
    if (!channel->getTopic().empty()) // print channel topic
        sendToClient(fd, ":localhost 332 " + nick + " " + channelName + " :" + channel->getTopic());
    else // print no channel topic
        sendToClient(fd, ":localhost 331 " + nick + " " + channelName + " :No topic is set");
    // display channel members
    sendToClient(fd, ":localhost 353 " + nick + " = " + channelName + " :" + channel->getMemberList());
    sendToClient(fd, ":localhost 366 " + nick + " " + channelName + " :End of /NAMES list");
}

void    Server::joinChannel(Client &client, const std::string &channelName, const std::string &channelKey)
{
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();

    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&')) // incorrect or no channel name provided
    {
        send_error(fd, "403", nick, channelName, "No such channel");
        return ;
    }
    Channel *channel = findChannel(channelName);
    if (channel == NULL) // new channel
    {
        Channel newChannel(channelName);
        channel_vector.push_back(newChannel);
        channel = &channel_vector.back();
        channel->addOperator(&client); // !!
        channel->addMember(&client); // !!
        if (!channelKey.empty())
            channel->setKey(channelKey);
    }
    else // existing channel
    {
        if (channel->isMember(&client)) // already a member
            return ;
        if (channel->getUserLimit() > 0 && (int)channel->memberCount() >= channel->getUserLimit()) // user limit reached
        {
            send_error(fd, "471", nick, channelName, "Cannot join channel (+l)");
            return ;
        }
        if (channel->isInviteOnly() && !channel->isInvited(&client)) // not invited for an invit only channel
        {
            send_error(fd, "473", nick, channelName, "Cannot join channel (+i)");
            return ;
        }
        if (!channel->getKey().empty() && channel->getKey() != channelKey) // incorrect channel key
        {
            send_error(fd, "475", nick, channelName, "Cannot join channel (+k)");
            return ;
        }
        channel->addMember(&client);
        channel->removeInvite(&client);
    }
    announceJoin(client, channel, channelName);
}

void    Server::handleJoin(Client &client, std::stringstream &ss)
{
    std::string channelName;
    std::string channelKey;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();

    if (!(ss >> channelName))
    {
        send_error(fd, "461", nick, "JOIN", "Not enough parameters");
        return ;
    }
    ss >> channelKey;
    joinChannel(client, channelName, channelKey);
}
