#include "../Channel.hpp"
#include "../Client.hpp"
#include "../Server.hpp"

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
    int fd = client.get_client_fd();
    std::string nick = client.get_nickname();
    Client  *targetClient = findClientByNick(target);
    if (!targetClient)
    {
        send_error(fd, "401", nick, target, "No such nick/channel");
        return ;
    }
    std::string fullMsg = getClientPrefix(client) + " PRIVMSG " + target + " :" + message;
    sendToClient(targetClient->get_client_fd(), fullMsg);
}

void    Server::privmsgToChannel(Client &client, const std::string &target, const std::string &message)
{
    Channel *channel = findChannel(target);
    int fd = client.get_client_fd();
    std::string nick = client.get_nickname();
    if (!channel)
    {
        send_error(fd, "403", nick, target, "No such channel");
        return ;
    }
    if (!channel->isMember(&client))
    {
        send_error(fd, "404", nick, target, "Cannot send to channel");
        return ;
    }
    std::string fullMsg = getClientPrefix(client) + " PRIVMSG " + target + " :" + message;
    channel->broadcastMessage(fullMsg, &client);
}

void    Server::handlePrivmsg(Client &client, std::stringstream &ss)
{
    int fd = client.get_client_fd();
    std::string nick = client.get_nickname();
    std::string target;
    if (!(ss >> target))
    {
        send_error(fd, "411", nick, "PRIVMSG", "No recipient given");
        return ;
    }
    std::string message = extractMessage(ss);
    if (message.empty())
    {
        send_error(fd, "412", nick, "PRIVMSG", "No text to send");
        return ;
    }
    if (target[0] == '#' || target[0] == '&')
        privmsgToChannel(client, target, message);
    else
        privmsgToUser(client, target, message);
}
