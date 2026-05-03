#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <poll.h>
#include <string>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server {
	private:
		int port;
		int server_socket;
		std::string passwd;
		std::vector<struct pollfd> sockets;
		std::vector<Client> client_vector;
		std::vector<Channel> channel_vector;
	public:
		Server(std::string port, std::string _passwd);
		void server_init();
		void main_loop();
		void accept_new_client();
		void receive_new_data(int i);
		void parse_buffer(Client &client);
		void remove_a_client(Client &client);
		void close_clients();
		void send_error(int client_fd, std::string code, std::string nickname, std::string command, std::string message);
		
		// -------------------- handleJoin -----------------------
		
		void    joinChannel(Client &client, const std::string &chanName, const std::string &chanKey);
		void    announceJoin(Client &client, Channel *channel, const std::string &chanName);
		
		// -------------------- handlePrivmsg -----------------------
		std::string	extractMessage(std::stringstream &ss);
		void	privmsgToUser(Client &client, const std::string &target, const std::string &message);
		void	privmsgToChannel(Client &client, const std::string &target, const std::string &message);
		
		
		// -------------------- Utils -----------------------

		std::string	getClientPrefix(Client &client);
		Channel	*findChannel(const std::string &name);
		Client	*findClientByNick(const std::string &nickname);
		void	sendToClient(int fd, const std::string &message);
		// void	removeClientFromAllChannels(Client &client);
		void	removeClientFromAllChannels(Client &client, const std::string &reason);

		void	handleCommands(Client &client, std::string &command, std::stringstream &ss);
		
		// -------------------- Commands -----------------------

		void	handlePrivmsg(Client &client, std::stringstream &ss);
		void	handlePing(Client &client, std::stringstream &ss);
		// void	handleQuit(Client &client);
		void	handleQuit(Client &client, std::stringstream &ss);

		// -------------------- userCommands -----------------------

		void	handleJoin(Client &client, std::stringstream &ss);
		void	handlePart(Client &client, std::stringstream &ss);

		// -------------------- operatorCommands -----------------------
		void	handleKick(Client &client, std::stringstream &ss);
		void	handleInvite(Client &client, std::stringstream &ss);
		void	handleTopic(Client &client, std::stringstream &ss);
		void	handleMode(Client &client, std::stringstream &ss);
};