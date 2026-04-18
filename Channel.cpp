#include "Channel.hpp"

Channel::Channel() {}

Channel::Channel(std::string &name) : name(name) {}

Channel::~Channel() {}
		
// getters
std::string Channel::getName() const
{
    return (this->name);
}

std::string	Channel::getPass() const {
    return (this->pass);
}

std::string	Channel::getTopic() const {
    return (this->topic);
}

int	Channel::getUserLimit() const {
    return (this->userLimit);
}

bool	Channel::isInviteOnly() const {
    return (this->inviteOnly);
}

bool	Channel::isTopicRestricted() const {
    return (this->topicRestricted);
}

// setters
void	Channel::set_topic(std::string &topic) {
    this->topic = topic;
}

void	Channel::set_pass(std::string &pass) {
    this->pass = pass;
}

void	Channel::set_user_limit(int userLimit) {
    this->userLimit = userLimit;
}

void	Channel::set_invit_only(bool inviteOnly) {
    this->inviteOnly = inviteOnly;
}

void	Channel::set_topic_restricted(bool topicRestricted) {
    this->topicRestricted = topicRestricted;
}
