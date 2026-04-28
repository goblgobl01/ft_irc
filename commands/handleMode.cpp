#include "../Channel.hpp"
#include "../Client.hpp"
#include "../Server.hpp"

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
    if (mode.empty() || mode.size() != 2 || (mode[0] != '+' && mode[0] != '-'))
    {
        std::cout << "invalid Mode !" << std::endl;
        return ;
    }

    std::string arg;
    ss >> arg;
    if (mode[0] == '+')
    {
        if (mode[1] == 'i')
            channel->setInviteOnly(true);
        else if (mode[1] == 't')
            channel->setTopicRestricted(true);
        else if (mode[1] == 'k')
            channel->setKey(arg);
        else if (mode[1] == 'o')
            channel->addOperator(&client);
        else if (mode[1] == 'l')
            channel->setUserLimit(0);
        else
        {
            std::cout << "invalid Mode !" << std::endl;
            return ;
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
            channel->removeOperator(&client);
        else if (mode[1] == 'l')
        {

            int limit = std::atoi(arg.c_str());
            channel->setUserLimit(limit);
        }
        else
        {
            std::cout << "invalid Mode !" << std::endl;
            return ;
        }
    }
}
