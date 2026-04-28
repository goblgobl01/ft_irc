#include "../Channel.hpp"
#include "../Client.hpp"
#include "../Server.hpp"

void	Server::handlePart(Client &client, std::stringstream &ss)
{
	std::string	nick = client.get_nickname();
	std::string	channels_str;
	int			fd = client.get_client_fd();
	if (!(ss >> channels_str))
	{
		send_error(fd, "461", nick, "PART", "Not enough parameters");
		return ;
	}
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
		{
			send_error(fd, "403", nick, chanName, "No such channel");
			continue ;
		}
		if (!channel->isMember(&client))
		{
			send_error(fd, "442", nick, chanName, "You're not on that channel");
			continue ;
		}
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
