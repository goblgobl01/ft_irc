#include "Channel.hpp"
#include "Server.hpp"

void	Server::handlePart(Client &client, std::stringstream &ss)
{
	std::string	channels_str;
	if (!(ss >> channels_str))
        SEND_ERROR_RETURN(client, "461", "PART", "Not enough parameters");
	// if (!(ss >> channels_str))
	// {
	// 	send_error(client.get_client_fd(), "461", client.get_nickname(), "PART", "Not enough parameters");
	// 	return ;
	// }
	std::string	reason;
	std::getline(ss, reason);
	size_t	colon = reason.find(':');
	if (colon != std::string::npos)
		reason = reason.substr(colon + 1);
	else
	{
		size_t	first = reason.find_first_not_of(" ");
		if (first != std::string::npos)
			reason = reason.substr(first);
		else
			reason = "";
	}
	std::stringstream	cs(channels_str);
	std::string			chanName;
	while (std::getline(cs, chanName, ','))
	{
		Channel	*channel = findChannel(chanName);
		if (!channel)
            SEND_ERROR_CONTINUE(client, "403", chanName, "No such channel");
		if (!channel->isMember(&client))
            SEND_ERROR_CONTINUE(client, "442", chanName, "You're not on that channel");
		// if (!channel)
		// {
		// 	send_error(client.get_client_fd(), "403", client.get_nickname(), chanName, "No such channel");
		// 	continue ;
		// }
		// if (!channel->isMember(&client))
		// {
		// 	send_error(client.get_client_fd(), "442", client.get_nickname(), chanName, "You're not on that channel");
		// 	continue ;
		// }
		std::string	partMsg = getClientPrefix(client) + " PART " + chanName;
		if (!reason.empty())
			partMsg += " :" + reason;
		channel->broadcastMessage(partMsg, NULL);
		channel->removeMember(&client);
		for (size_t i = 0; i < channel_vector.size(); i++)
		{
			if (channel_vector[i].getName() == chanName && channel_vector[i].memberCount() == 0)
			{
				channel_vector.erase(channel_vector.begin() + i);
				break ;
			}
		}
	}
}

void    Server::syncProtocol(Client &client, Channel *channel, const std::string &chanName)
{
    channel->broadcastMessage(getClientPrefix(client) + " JOIN " + chanName, NULL);
    if (!channel->getTopic().empty())
        sendToClient(client.get_client_fd(), ":localhost 332 " + client.get_nickname() + " " + chanName + " :" + channel->getTopic());
    else
        sendToClient(client.get_client_fd(), ":localhost 331 " + client.get_nickname() + " " + chanName + " :No topic is set");
    sendToClient(client.get_client_fd(), ":localhost 353 " + client.get_nickname() + " = " + chanName + " :" + channel->getMemberList());
    sendToClient(client.get_client_fd(), ":localhost 366 " + client.get_nickname() + " " + chanName + " :End of /NAMES list");
}

void    Server::joinChannel(Client &client, const std::string &chanName, const std::string &chanKey)
{
    if (chanName.empty() || (chanName[0] != '#' && chanName[0] != '&'))
        SEND_ERROR_RETURN(client, "403", chanName, "No such channel");
    // if (chanName.empty() || (chanName[0] != '#' && chanName[0] != '&'))
    // {
    //     send_error(client.get_client_fd(), "403", client.get_nickname(), chanName, "No such channel");
    //     return;
    // }
    Channel *channel = findChannel(chanName);
    bool isNewChannel = (channel == NULL);
    if (isNewChannel)
    {
        Channel newChannel(chanName);
        channel_vector.push_back(newChannel);
        channel = &channel_vector.back();
    }
    if (channel->isMember(&client))
        return ;
    if (!isNewChannel)
    {
        if (channel->getUserLimit() > 0 && (int)channel->memberCount() >= channel->getUserLimit())
            SEND_ERROR_RETURN(client, "471", chanName, "Cannot join channel (+l)");
        if (channel->isInviteOnly() && !channel->isInvited(&client))
            SEND_ERROR_RETURN(client, "473", chanName, "Cannot join channel (+i)");
        if (!channel->getKey().empty() && channel->getKey() != chanKey)
            SEND_ERROR_RETURN(client, "475", chanName, "Cannot join channel (+k)");
        // if (channel->getUserLimit() > 0 && (int)channel->memberCount() >= channel->getUserLimit())
        // {
        //     send_error(client.get_client_fd(), "471", client.get_nickname(), chanName, "Cannot join channel (+l)");
        //     return ;
        // }
        // if (channel->isInviteOnly() && !channel->isInvited(&client))
        // {
        //     send_error(client.get_client_fd(), "473", client.get_nickname(), chanName, "Cannot join channel (+i)");
        //     return ;
        // }
        // if (!channel->getKey().empty() && channel->getKey() != chanKey)
        // {
        //     send_error(client.get_client_fd(), "475", client.get_nickname(), chanName, "Cannot join channel (+k)");
        //     return ;
        // }
    }
    channel->addMember(&client);
    channel->removeInvite(&client);
    if (isNewChannel)
        channel->addOperator(&client);
    syncProtocol(client, channel, chanName);
}

void    Server::handleJoin(Client &client, std::stringstream &ss)
{
    std::string channels_str;
    std::string keys_str;
    if (!(ss >> channels_str))
        SEND_ERROR_RETURN(client, "461", "JOIN", "Not enough parameters");
    // if (!(ss >> channels_str))
    // {
    //     send_error(client.get_client_fd(), "461", client.get_nickname(), "JOIN", "Not enough parameters");
    //     return;
    // }
    ss >> keys_str;
    std::vector<std::string> channels;
    std::vector<std::string> keys;
    std::string tmp;
    std::stringstream cs(channels_str);
    while (std::getline(cs, tmp, ','))
        channels.push_back(tmp);
    std::stringstream ks(keys_str);
    while (std::getline(ks, tmp, ','))
        keys.push_back(tmp);
    for (size_t i = 0; i < channels.size(); i++)
    {
        std::string chanKey = (i < keys.size()) ? keys[i] : "";
        joinChannel(client, channels[i], chanKey);
    }
}
