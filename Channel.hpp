#ifndef CHANNEL_H
# define CHANNEL_H

#include <iostream>
#include <vector>
#include <string>

class   Client;

class   Channel
{
	private:
		int		userLimit;
		bool	inviteOnly;
		bool	topicRestricted;
		std::string	name;
		std::string	pass; // key
		std::string	topic;
		std::vector<Client>	members;
		std::vector<Client>	operators;
		std::vector<Client>	inviteList;
	public:
		Channel();
		Channel(std::string &name);
		~Channel();
		
		// getters
		std::string	getName() const;
		std::string	getPass() const;
		std::string	getTopic() const;
		int	getUserLimit() const;
		bool	isInviteOnly() const;
		bool	isTopicRestricted() const;
		
		// setters
		void	set_topic(std::string &topic);
		void	set_pass(std::string &pass);
		void	set_user_limit(int user_limit);
		void	set_invit_only(bool is_invite_only);
		void	set_topic_restricted(bool is_topic_restricted);

		// member management
		void	addMember();
		void	removeMember();
		void	isMember();
		void	memberCount();
		void	getMemberList();

		// operator management
		void	addOperator();
		void	removeOperator();
		void	isOperator();

		// invite management
		void	addInvite();
		void	removeInvite();
		void	isInvited();

		// broadcast
		void	broadcastMessage(std::string &message, std::vector<Client> exclude);
};

#endif
