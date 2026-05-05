#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Bot.hpp"

std::string Server::extractMessage(std::stringstream &ss)
{
    std::string message;
    std::getline(ss, message);
    size_t first = message.find_first_not_of(" ");
    if (first == std::string::npos)
        return ("");
    if (message[first] == ':')
        return (message.substr(first + 1));
    return (message.substr(first));
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
    std::string target;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();
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


    //  BOT 

    if (target == "IrcBot" && !message.empty() && message[0] == '!')
    {
        std::string response = Bot::handleCommand(message, &client, this);
        std::istringstream rs(response);
        std::string line;
        while (std::getline(rs, line))
        {
            std::string reply = ":IrcBot!bot@server PRIVMSG " + nick + " :" + line + "\r\n";
            sendToClient(fd, reply);
        }
        return ;
    }
    //  END BOT  

    if (target[0] == '#' || target[0] == '&')
        privmsgToChannel(client, target, message);
    else
        privmsgToUser(client, target, message);
}
