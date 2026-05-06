#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

/*
        TOPIC <channel> [:<topic>]

    Behaviour:
      TOPIC #chan           view current topic
      TOPIC #chan :         clear the topic 
      TOPIC #chan :text    → set topic to "text"

    If the channel has mode +t (topicRestricted) only operators may change the topic; any member may still view it.

*/

void    Server::handleTopic(Client &client, std::stringstream &ss)
{
    std::string chanName;
    std::string nick = client.get_nickname();
    int         fd   = client.get_client_fd();

    if (!(ss >> chanName))
    {
        send_error(fd, "461", nick, "TOPIC", "Not enough parameters");
        return ;
    }

    //  Channel must exist 
    Channel *channel = findChannel(chanName);
    if (!channel)
    {
        send_error(fd, "403", nick, chanName, "No such channel");
        return ;
    }

    //  Client must be on channel 
    if (!channel->isMember(&client))
    {
        send_error(fd, "442", nick, chanName, "You're not on that channel");
        return ;
    }


    std::string rest;
    std::getline(ss, rest);

    size_t firstNonSpace = rest.find_first_not_of(" \t");

    // No topic argument: view mode 
    if (firstNonSpace == std::string::npos)
    {
        if (channel->getTopic().empty())
            sendToClient(fd, ":localhost 331 " + nick + " " + chanName + " :No topic is set");
        else
            sendToClient(fd, ":localhost 332 " + nick + " " + chanName + " :" + channel->getTopic());
        return ;
    }

    //  Topic argument provided: set/clear mode 

    // Check operator restriction before doing anything
    if (channel->isTopicRestricted() && !channel->isOperator(&client))
    {
        send_error(fd, "482", nick, chanName, "You're not channel operator");
        return ;
    }

    // Extract the new topic value
    std::string newTopic = rest.substr(firstNonSpace);

    // Strip leading colon (IRC trailing parameter convention: ":text" -> "text")
    if (!newTopic.empty() && newTopic[0] == ':')
        newTopic = newTopic.substr(1);

    // Setthe topic
    channel->setTopic(newTopic);

    // Broadcast to ALL members (including the setter) so every client updates
    channel->broadcastMessage(getClientPrefix(client) + " TOPIC " + chanName + " :" + newTopic, NULL);
}