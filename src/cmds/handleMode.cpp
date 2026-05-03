#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void    Server::handleMode(Client &client, std::stringstream &ss)
{
    std::string target;
    std::string nick = client.get_nickname();
    int         fd = client.get_client_fd();
    if (!(ss >> target)) // if no channel name provided
    {
        send_error(fd, "461", nick, "MODE", "Not enough parameters");
        return ;
    }
    if (target[0] != '#' && target[0] != '&') // if the channel name do not start with # ro &
    {
        sendToClient(client.get_client_fd(), ":localhost 221 " + client.get_nickname() + " +");
        return ;
    }

    Channel *channel = findChannel(target);
    if (!channel) // if the channel do not exist
    {
        send_error(fd, "403", nick, target, "No such channel");
        return ;
    }
    
    std::string mode;
    ss >> mode;
    if (mode.empty() || mode.size() != 2 || (mode[0] != '+' && mode[0] != '-')
        || (mode[1] != 'i' && mode[1] != 't' && mode[1] != 'k' && mode[1] != 'o' && mode[1] != 'l'))
    {
        send_error(fd, "472", nick, target, "Invalid mode");
        return ;
    }
    std::string arg;
    ss >> arg;
    if ((mode == "+k" || mode == "+l" || mode == "+o" || mode == "-o") && arg.empty())
    {
        send_error(fd, "461", nick, target, "Not enough parameters");
        return ;
    }
    if (!channel->isMember(&client)) // only mambers can change modes
    {
        send_error(fd, "442", nick, target, "You're not on that channel");
        return ;
    }
    if (!channel->isOperator(&client)) // only operators can change modes
    {
        send_error(fd, "482", nick, target, "You're not channel operator");
        return ;
    }
    if (mode[0] == '+')
    {
        if (mode[1] == 'i')
            channel->setInviteOnly(true);
        else if (mode[1] == 't')
            channel->setTopicRestricted(true);
        else if (mode[1] == 'k')
            channel->setKey(arg);
        else if (mode[1] == 'o')
        {
            Client  *target = findClientByNick(arg);
            if (!target)
            {
                send_error(fd, "401", nick, arg, "No such nick");
                return ;
            }
            if (!channel->isMember(target))
            {
                send_error(fd, "482", nick, target->get_nickname(), "is not a member");
                return ;
            }
            channel->addOperator(target);
        }
        else
        {
            int limit = std::atoi(arg.c_str());
            channel->setUserLimit(limit);
        }
    }
    else
    {
        if (mode[1] == 'i')
            channel->setInviteOnly(false);
        else if (mode[1] == 't')
            channel->setTopicRestricted(false);
        else if (mode[1] == 'k')
            channel->setKey("");
        else if (mode[1] == 'o')
        {
            Client  *target = findClientByNick(arg);
            if (!target)
            {
                send_error(fd, "401", nick, arg, "No such nick");
                return ;
            }
            if (!channel->isMember(target))
            {
                send_error(fd, "482", nick, target->get_nickname(), "is not a member");
                return ;
            }
            channel->removeOperator(target);
        }
        else
            channel->setUserLimit(0);
    }
    std::string message = getClientPrefix(client) + " MODE " + target + " " + mode;
    if (!arg.empty())
        message += " " + arg;
    channel->broadcastMessage(message, NULL);
}
