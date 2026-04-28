#include "../Channel.hpp"
#include "../Client.hpp"
#include "../Server.hpp"

void	Server::removeClientFromAllChannels(Client &client) {
	for (size_t i = 0; i < channel_vector.size();)
	{
		if (channel_vector[i].isMember(&client))
		{
			channel_vector[i].broadcastMessage(getClientPrefix(client) + " QUIT :Quit", &client);
			channel_vector[i].removeMember(&client);
		}
		if (channel_vector[i].memberCount() == 0)
			channel_vector.erase(channel_vector.begin() + i);
		else
			i++;
	}
}

void	Server::handleQuit(Client &client)
{
	removeClientFromAllChannels(client);
	remove_a_client(client);
}
