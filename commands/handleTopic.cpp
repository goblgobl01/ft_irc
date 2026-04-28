#include "../Server.hpp"
#include "../Client.hpp"
#include "../Channel.hpp"

void    Server::handleTopic(Client &client, std::stringstream &ss)
{
    std::string chanName;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();
    if (!(ss >> chanName)) // parse channel name
    {
        send_error(fd, "461", nick, "TOPIC", "Not enough parameters");
        return ;
    }
    Channel *channel = findChannel(chanName); // check if the channel exists
    if (!channel)
    {
        send_error(fd, "403", nick, chanName, "No such channel");
        return ;
    }
    if (!channel->isMember(&client)) // check if the client is a channel member
    {
        send_error(fd, "442", nick, chanName, "You're not on that channel");
        return ;
    }
    std::string rest;
    std::getline(ss, rest);
    size_t first_nonspace = rest.find_first_not_of(" ");
    if (first_nonspace == std::string::npos)
    {
        if (channel->getTopic().empty())
            sendToClient(client.get_client_fd(), ":localhost 331 " + client.get_nickname() + " " + chanName + " :No topic is set");
        else
            sendToClient(client.get_client_fd(), ":localhost 332 " + client.get_nickname() + " " + chanName + " :" + channel->getTopic());
        return ;
    }
    if (channel->isTopicRestricted() && !channel->isOperator(&client)) // check permission for setting topic
    {
        send_error(fd, "482", nick, chanName, "You're not channel operator");
        return ;
    }
    std::string newTopic = rest.substr(first_nonspace);  // set and broadcast the new topic
    if (newTopic[0] == ':')
        newTopic = newTopic.substr(1);
    channel->setTopic(newTopic);
    channel->broadcastMessage(getClientPrefix(client) + " TOPIC " + chanName + " :" + newTopic, NULL);
}
