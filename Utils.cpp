#include "Channel.hpp"
#include "Server.hpp"

std::string Server::getClientPrefix(Client &client) {
	return (":" + client.get_nickname() + "!" + client.get_username() + "@localhost");
}

Channel *Server::findChannel(const std::string &name) {
	for (size_t i = 0; i < channel_vector.size(); i++)
	{
		if (channel_vector[i].getName() == name)
			return (&channel_vector[i]);
	}
	return (NULL);
}

Client  *Server::findClientByNick(const std::string &nickname) {
	for (size_t i = 0; i < client_vector.size(); i++)
	{
		if (client_vector[i].get_nickname() == nickname)
			return (&client_vector[i]);
	}
	return (NULL);
}

void    Server::sendToClient(int fd, const std::string &message) {
	std::string msg = message + "\r\n";
	send(fd, msg.c_str(), msg.size(), 0);
}

void	Server::removeClientFromAllChannels(Client &client) {
	for (size_t i = 0; i < channel_vector.size();)
	{
		if (channel_vector[i].isMember(&client))
		{
			channel_vector[i].broadcastMessage(getClientPrefix(client) + " QUIT :Quit", &client);
			channel_vector[i].removeMember(&client); // TO DO
		}
		if (channel_vector[i].memberCount() == 0)
			channel_vector.erase(channel_vector.begin() + i);
		else
			i++;
	}
}

void	Server::handleCommands(Client &client, std::string &command, std::stringstream &ss) {
	if (!client.is_registered())
    {
        send_error(client.get_client_fd(), "451", "*", command, "You have not registered");
        return;
    }
	if (command == "JOIN")
		handleJoin(client, ss);
	else if (command == "PRIVMSG")
		handlePrivmsg(client, ss);
	else if (command == "PART")
		handlePart(client, ss);
	else if (command == "QUIT")
		handleQuit(client);
	else if (command == "PING")
		handlePing(client, ss);
	else if (command == "KICK")
		handleKick(client, ss);
	else if (command == "INVITE")
		handleInvite(client, ss);
	else if (command == "TOPIC")
		handleTopic(client, ss);
	else if (command == "MODE")
		handleMode(client, ss);
	else
		send_error(client.get_client_fd(), "421", client.get_nickname(), command, "Unkown command");
}
