#ifndef CHANNEL_H
# define CHANNEL_H

#include <iostream>
#include <vector>
#include <string>

class   Client;

class   Channel
{
	private:
		int						userLimit;
		bool					inviteOnly;
		bool					topicRestricted;
		std::string				key;
		std::string				name;
		std::string				topic;
		std::vector<Client *>	members;
		std::vector<Client *>	operators;
		std::vector<Client *>	inviteList;

	public:		
		Channel();
		Channel(const std::string &name);
		~Channel();


		std::string	getName() const;
		std::string	getKey() const;
		std::string	getTopic() const;
		int			getUserLimit() const;
		bool		isInviteOnly() const;
		bool    	isTopicRestricted() const;


		void    setTopic(const std::string &topic);
		void    setKey(const std::string &key);
		void    setUserLimit(int limit);
		void    setInviteOnly(bool status);
		void    setTopicRestricted(bool status);


		bool	isMember(Client *client) const;
		bool	isOperator(Client *client) const;
		bool	isInvited(Client *client) const;
		void    addInvite(Client *client);
		

		std::string	getMemberList() const;
		size_t		memberCount() const;
		void		removeOperator(Client *client);
		void		addMember(Client *client);
		void		removeMember(Client *client);
		void		addOperator(Client *client);
		void		removeInvite(Client *client);
		void		broadcastMessage(const std::string &message, Client *exclude);
};

#endif
