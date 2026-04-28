    if (!(ss >> mode)) // get mode (+i, +t, +k, +k or +l)
    {
        if ()
        if (mode[0] == '+')
        else if ()
        std::string modes = "+";
        std::string params = "";
        if (channel->isInviteOnly())
            modes += "i";
        if (channel->isTopicRestricted())
            modes += "t";
        if (!channel->getKey().empty())
        {
            modes += "k";
            params += " " + channel->getKey();
        }
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


    if (!channel->isMember(&client)) // the client is not a member of the channel
    {
        send_error(fd, "442", nick, target, "You're not on that channel");
        return ;
    }
    if (!channel->isOperator(&client)) // the client is not a channel operator
    {
        send_error(fd, "482", nick, target, "You're not channel operator");
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
                    send_error(fd, "461", nick, "MODE", "Not enough parameters");
                    continue ;
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
                send_error(fd, "461", nick, "MODE", "Not enough parameters");
                continue ;
            }
            Client *targetClient = findClientByNick(params[paramIdx]);
            if (!targetClient || !channel->isMember(targetClient))
            {
                send_error(client.get_client_fd(), "441", client.get_nickname(), params[paramIdx] + " " + target, "They aren't on that channel");
                paramIdx++;
                continue ;
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
                    send_error(fd, "461", nick, "MODE", "Not enough parameters");
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