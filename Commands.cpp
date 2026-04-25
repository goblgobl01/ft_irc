#include "Channel.hpp"
#include "Server.hpp"

void	Server::handleQuit(Client &client)
{
	removeClientFromAllChannels(client);
	remove_a_client(client);
}

void    Server::handlePing(Client &client, std::stringstream &ss)
{
	std::string token;
	if (!(ss >> token))
        SEND_ERROR_RETURN(client, "409", "PING", "No origin specified");
	// if (!(ss >> token))
	// {
	// 	send_error(client.get_client_fd(), "409", client.get_nickname(), "PING", "No origin specified");
	// 	return ;
	// }
	sendToClient(client.get_client_fd(), "PONG localhost :" + token);
}

std::string Server::extractMessage(std::stringstream &ss)
{
    std::string message;
    std::getline(ss, message);
    size_t  start = message.find(':');
    if (start != std::string::npos)
        return (message.substr(start + 1));
    size_t  first = message.find_first_not_of(" ");
    if (first != std::string::npos)
        return (message.substr(first));
    return ("");
}

void    Server::privmsgToUser(Client &client, const std::string &target, const std::string &message)
{
    Client  *targetClient = findClientByNick(target);
    if (!targetClient)
        SEND_ERROR_RETURN(client, "401", target, "No such nick/channel");
    // if (!targetClient)
    // {
    //     send_error(client.get_client_fd(), "401", client.get_nickname(), target, "No such nick/channel");
    //     return ;
    // }
    std::string fullMsg = getClientPrefix(client) + " PRIVMSG " + target + " :" + message;
    sendToClient(targetClient->get_client_fd(), fullMsg);
}

void    Server::privmsgToChannel(Client &client, const std::string &target, const std::string &message)
{
    Channel *channel = findChannel(target);
    if (!channel)
        SEND_ERROR_RETURN(client, "403", target, "No such channel");
    if (!channel->isMember(&client))
        SEND_ERROR_RETURN(client, "404", target, "Cannot send to channel");
    // if (!channel)
    // {
    //     send_error(client.get_client_fd(), "403", client.get_nickname(), target, "No such channel");
    //     return ;
    // }
    // if (!channel->isMember(&client))
    // {
    //     send_error(client.get_client_fd(), "404", client.get_nickname(), target, "Cannot send to channel");
    //     return ;
    // }
    std::string fullMsg = getClientPrefix(client) + " PRIVMSG " + target + " :" + message;
    channel->broadcastMessage(fullMsg, &client);
}

void    Server::handlePrivmsg(Client &client, std::stringstream &ss)
{
    std::string target;
    if (!(ss >> target))
        SEND_ERROR_RETURN(client, "411", "PRIVMSG", "No recipient given");
    // if (!(ss >> target))
    // {
    //     send_error(client.get_client_fd(), "411", client.get_nickname(), "PRIVMSG", "No recipient given");
    //     return ;
    // }
    std::string message = extractMessage(ss);
    if (message.empty())
        SEND_ERROR_RETURN(client, "412", "PRIVMSG", "No text to send");
    // if (message.empty())
    // {
    //     send_error(client.get_client_fd(), "412", client.get_nickname(), "PRIVMSG", "No text to send");
    //     return ;
    // }
    if (target[0] == '#' || target[0] == '&')
        privmsgToChannel(client, target, message);
    else
        privmsgToUser(client, target, message);
}
