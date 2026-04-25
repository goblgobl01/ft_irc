#include "Channel.hpp"
#include "Server.hpp"

void    Server::handleKick(Client &client, std::stringstream &ss)
{
    std::string chanName, targetNick;
    if (!(ss >> chanName >> targetNick))
    {
        send_error(client.get_client_fd(), "461", client.get_nickname(), "KICK", "Not enough parameters");
        return ;
    }
    std::string reason;
    std::getline(ss, reason);
    size_t colon = reason.find(':');
    if (colon != std::string::npos)
        reason = reason.substr(colon + 1);
    else
    {
        size_t first = reason.find_first_not_of(" ");
        if (first != std::string::npos)
            reason = reason.substr(first);
        else
            reason = client.get_nickname();
    }
    Channel *channel = findChannel(chanName);
    if (!channel)
    {
        send_error(client.get_client_fd(), "403", client.get_nickname(), chanName, "No such channel");
        return ;
    }
    if (!channel->isMember(&client))
    {
        send_error(client.get_client_fd(), "442", client.get_nickname(), chanName, "You're not on that channel");
        return ;
    }
    if (!channel->isOperator(&client))
    {
        send_error(client.get_client_fd(), "482", client.get_nickname(), chanName, "You're not channel operator");
        return ;
    }
    Client *target = findClientByNick(targetNick);
    if (!target || !channel->isMember(target))
    {
        send_error(client.get_client_fd(), "441", client.get_nickname(), targetNick + " " + chanName, "They aren't on that channel");
        return ;
    }
    channel->broadcastMessage(
        getClientPrefix(client) + " KICK " + chanName + " " + targetNick + " :" + reason, NULL);
    channel->removeMember(target);
    if (channel->memberCount() == 0)
    {
        for (size_t i = 0; i < channel_vector.size(); i++)
        {
            if (channel_vector[i].getName() == chanName)
            {
                channel_vector.erase(channel_vector.begin() + i);
                break ;
            }
        }
    }
}

void    Server::handleInvite(Client &client, std::stringstream &ss)
{
    std::string targetNick, chanName;
    if (!(ss >> targetNick >> chanName))
    {
        send_error(client.get_client_fd(), "461", client.get_nickname(), "INVITE", "Not enough parameters");
        return ;
    }
    Channel *channel = findChannel(chanName);
    if (!channel)
    {
        send_error(client.get_client_fd(), "403", client.get_nickname(), chanName, "No such channel");
        return ;
    }
    if (!channel->isMember(&client))
    {
        send_error(client.get_client_fd(), "442", client.get_nickname(), chanName, "You're not on that channel");
        return ;
    }
    if (channel->isInviteOnly() && !channel->isOperator(&client))
    {
        send_error(client.get_client_fd(), "482", client.get_nickname(), chanName, "You're not channel operator");
        return ;
    }
    Client *target = findClientByNick(targetNick);
    if (!target)
    {
        send_error(client.get_client_fd(), "401", client.get_nickname(), targetNick, "No such nick/channel");
        return ;
    }
    if (channel->isMember(target))
    {
        send_error(client.get_client_fd(), "443", client.get_nickname(), targetNick + " " + chanName, "is already on channel");
        return ;
    }
    channel->addInvite(target);
    sendToClient(client.get_client_fd(), ":localhost 341 " + client.get_nickname() + " " + targetNick + " " + chanName);
    sendToClient(target->get_client_fd(), getClientPrefix(client) + " INVITE " + targetNick + " " + chanName);
}

void    Server::handleTopic(Client &client, std::stringstream &ss)
{
    std::string chanName;
    if (!(ss >> chanName))
    {
        send_error(client.get_client_fd(), "461", client.get_nickname(), "TOPIC", "Not enough parameters");
        return ;
    }

    Channel *channel = findChannel(chanName);
    if (!channel)
    {
        send_error(client.get_client_fd(), "403", client.get_nickname(), chanName, "No such channel");
        return ;
    }
    if (!channel->isMember(&client))
    {
        send_error(client.get_client_fd(), "442", client.get_nickname(), chanName, "You're not on that channel");
        return ;
    }
    std::string rest;
    std::getline(ss, rest);
    size_t first_nonspace = rest.find_first_not_of(" ");
    if (first_nonspace == std::string::npos)
    {
        if (channel->getTopic().empty())
            sendToClient(client.get_client_fd(), ":localhost 331 " + client.get_nickname() + " " + chanName + " :No topic is set");
        else
            sendToClient(client.get_client_fd(), ":localhost 332 " + client.get_nickname() + " " + chanName + " :" + channel->getTopic());
        return ;
    }
    if (channel->isTopicRestricted() && !channel->isOperator(&client))
    {
        send_error(client.get_client_fd(), "482", client.get_nickname(), chanName, "You're not channel operator");
        return ;
    }
    std::string newTopic = rest.substr(first_nonspace);
    if (newTopic[0] == ':')
        newTopic = newTopic.substr(1);
    channel->setTopic(newTopic);
    channel->broadcastMessage(
        getClientPrefix(client) + " TOPIC " + chanName + " :" + newTopic, NULL);
}

void    Server::handleMode(Client &client, std::stringstream &ss)
{
    std::string target;
    if (!(ss >> target))
    {
        send_error(client.get_client_fd(), "461", client.get_nickname(), "MODE", "Not enough parameters");
        return ;
    }
    if (target[0] != '#' && target[0] != '&')
    {
        sendToClient(client.get_client_fd(), ":localhost 221 " + client.get_nickname() + " +");
        return ;
    }
    Channel *channel = findChannel(target);
    if (!channel)
    {
        send_error(client.get_client_fd(), "403", client.get_nickname(), target, "No such channel");
        return ;
    }
    std::string modeString;
    if (!(ss >> modeString))
    {
        std::string modes = "+";
        std::string params = "";
        if (channel->isInviteOnly()) modes += "i";
        if (channel->isTopicRestricted()) modes += "t";
        if (!channel->getKey().empty()) { modes += "k"; params += " " + channel->getKey(); }
        if (channel->getUserLimit() > 0)
        {
            modes += "l";
            std::stringstream ls;
            ls << channel->getUserLimit();
            params += " " + ls.str();
        }
        sendToClient(client.get_client_fd(), ":localhost 324 " + client.get_nickname() + " " + target + " " + modes + params);
        return ;
    }

    if (!channel->isMember(&client))
    {
        send_error(client.get_client_fd(), "442", client.get_nickname(), target, "You're not on that channel");
        return ;
    }
    if (!channel->isOperator(&client))
    {
        send_error(client.get_client_fd(), "482", client.get_nickname(), target, "You're not channel operator");
        return ;
    }
    std::vector<std::string> params;
    std::string p;
    while (ss >> p)
        params.push_back(p);
    bool adding = true;
    size_t paramIdx = 0;
    std::string appliedModes = "";
    std::string appliedParams = "";
    for (size_t i = 0; i < modeString.size(); i++)
    {
        char c = modeString[i];
        if (c == '+') { adding = true; continue; }
        if (c == '-') { adding = false; continue; }
        if (appliedModes.empty() || (adding && appliedModes[0] == '-') || (!adding && appliedModes[0] == '+'))
            appliedModes += (adding ? "+" : "-");
        if (c == 'i')
        {
            channel->setInviteOnly(adding);
            appliedModes += "i";
        }
        else if (c == 't')
        {
            channel->setTopicRestricted(adding);
            appliedModes += "t";
        }
        else if (c == 'k')
        {
            if (adding)
            {
                if (paramIdx >= params.size())
                {
                    send_error(client.get_client_fd(), "461", client.get_nickname(), "MODE", "Not enough parameters");
                    continue;
                }
                channel->setKey(params[paramIdx]);
                appliedModes += "k";
                appliedParams += " " + params[paramIdx];
                paramIdx++;
            }
            else
            {
                channel->setKey("");
                appliedModes += "k";
            }
        }
        else if (c == 'o')
        {
            if (paramIdx >= params.size())
            {
                send_error(client.get_client_fd(), "461", client.get_nickname(), "MODE", "Not enough parameters");
                continue;
            }
            Client *targetClient = findClientByNick(params[paramIdx]);
            if (!targetClient || !channel->isMember(targetClient))
            {
                send_error(client.get_client_fd(), "441", client.get_nickname(), params[paramIdx] + " " + target, "They aren't on that channel");
                paramIdx++;
                continue;
            }
            if (adding)
                channel->addOperator(targetClient);
            else
                channel->removeOperator(targetClient);
            appliedModes += "o";
            appliedParams += " " + params[paramIdx];
            paramIdx++;
        }
        else if (c == 'l')
        {
            if (adding)
            {
                if (paramIdx >= params.size())
                {
                    send_error(client.get_client_fd(), "461", client.get_nickname(), "MODE", "Not enough parameters");
                    continue ;
                }
                std::stringstream ls(params[paramIdx]);
                int limit;
                if (!(ls >> limit) || limit <= 0)
                {
                    paramIdx++;
                    continue ;
                }
                channel->setUserLimit(limit);
                appliedModes += "l";
                appliedParams += " " + params[paramIdx];
                paramIdx++;
            }
            else
            {
                channel->setUserLimit(0);
                appliedModes += "l";
            }
        }
        else
            send_error(client.get_client_fd(), "472", client.get_nickname(), std::string(1, c), "is unknown mode char to me");
    }
    if (!appliedModes.empty())
        channel->broadcastMessage(getClientPrefix(client) + " MODE " + target + " " + appliedModes + appliedParams, NULL);
}
