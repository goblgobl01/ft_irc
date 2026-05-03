#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void	Server::handleCommands(Client &client, std::string &command, std::stringstream &ss)
{
	if (command == "QUIT")
	{
		handleQuit(client, ss);
		return ;
	}
	if (!client.is_registered())
    {
        send_error(client.get_client_fd(), "451", "*", command, "You have not registered");
        return;
    }
	if (command == "PING")
		handlePing(client, ss);
	else if (command == "JOIN")
		handleJoin(client, ss);
	else if (command == "PRIVMSG")
		handlePrivmsg(client, ss);
	else if (command == "PART")
		handlePart(client, ss);
	else if (command == "MODE")
		handleMode(client, ss);
	else if (command == "INVITE") // borz
		handleInvite(client, ss);
	else if (command == "KICK")
		handleKick(client, ss);
	else if (command == "TOPIC")
		handleTopic(client, ss);
	else
		send_error(client.get_client_fd(), "421", client.get_nickname(), command, "Unkown command");
}
