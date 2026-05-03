#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"

void	Server::handlePart(Client &client, std::stringstream &ss)
{
	std::string	reason;
	std::string	channelName;
	std::string	nick = client.get_nickname();
	int			fd = client.get_client_fd();
	if (!(ss >> channelName))
	{
		send_error(fd, "461", nick, "PART", "Not enough parameters");
		return ;
	}
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
	Channel	*channel = findChannel(channelName);
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
	std::string	partMessage = getClientPrefix(client) + " PART " + channelName;
	if (!reason.empty())
		partMessage += " :" + reason;
	channel->broadcastMessage(partMessage, NULL);
	channel->removeMember(&client);
	for (size_t i = 0; i < channel_vector.size(); i++)
	{
		if (channel_vector[i].getName() == channelName && channel_vector[i].memberCount() == 0)
		{
			channel_vector.erase(channel_vector.begin() + i);
			break ;
		}
	}
}
