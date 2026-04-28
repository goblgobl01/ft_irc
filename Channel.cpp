#include "Client.hpp"
#include "Channel.hpp"
#include <sys/socket.h>

Channel::Channel() : userLimit(0), inviteOnly(false), topicRestricted(false) {}
Channel::Channel(const std::string &name) : userLimit(0), inviteOnly(false), topicRestricted(false), name(name) {}
Channel::~Channel() {}

std::string Channel::getName() const { return (this->name); }
std::string Channel::getKey() const { return (this->key); }
std::string Channel::getTopic() const { return (this->topic); }
int         Channel::getUserLimit() const { return (this->userLimit); }
bool	    Channel::isInviteOnly() const { return (this->inviteOnly); }
bool        Channel::isTopicRestricted() const { return this->topicRestricted; }

void    Channel::setTopic(const std::string &topic) { this->topic = topic; }
void    Channel::setKey(const std::string &key) { this->key = key; }
void    Channel::setUserLimit(int limit) { this->userLimit = limit; }
void    Channel::setInviteOnly(bool status) { this->inviteOnly = status; }
void    Channel::setTopicRestricted(bool status) { this->topicRestricted = status; }

bool    Channel::isMember(Client *client) const {
    for (size_t i = 0; i < members.size(); i++)
    {
        if (members[i] == client)
            return (true);
    }
    return (false);
}
bool    Channel::isOperator(Client *client) const {
    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i] == client)
            return (true);
    }
    return (false);
}
bool    Channel::isInvited(Client *client) const {
    for (size_t i = 0; i < inviteList.size(); i++)
    {
        if (inviteList[i] == client)
            return (true);
    }
    return (false);
}
void    Channel::addInvite(Client *client) {
    if (!isInvited(client))
        inviteList.push_back(client);
}

std::string Channel::getMemberList() const {
    std::string list;
    for (size_t i = 0; i < members.size(); i++)
    {
        if (isOperator(members[i]))
            list += "@";
        list += members[i]->get_nickname();
        if (i + 1 < members.size())
            list += " ";
    }
    return (list);
}
size_t      Channel::memberCount() const { return (members.size()); }
void    Channel::removeOperator(Client *client)
{
    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i] == client)
        {
            operators.erase(operators.begin() + i);
            break;
        }
    }
}
void    Channel::addMember(Client *client) {
    if (!isMember(client))
        members.push_back(client);
}
void    Channel::removeMember(Client *client) {
    for (size_t i = 0; i < members.size(); i++)
    {
        if (members[i] == client)
        {
            if (members[i] == client)
            {
                members.erase(members.begin() + i);
                break ;
            }
        }
        removeOperator(client);
    }
}
void    Channel::addOperator(Client *client) {
    if (!isOperator(client))
        operators.push_back(client);
}
void    Channel::removeInvite(Client *client) {
    for (size_t i = 0; i < inviteList.size(); i++)
    {
        if (inviteList[i] == client)
        {
            inviteList.erase(inviteList.begin() + i);
            break ;
        }
    }
}
void    Channel::broadcastMessage(const std::string &message, Client *exclude) {
    for (size_t i = 0; i < members.size(); i++)
    {
        if (members[i] != exclude)
        {
            std::string msg = message + "\r\n";
            send(members[i]->get_client_fd(), msg.c_str(), msg.size(), 0);
        }
    }
}
