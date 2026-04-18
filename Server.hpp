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

class Server {
	private:
		int port;
		int server_socket;
		std::string passwd;
		std::vector<struct pollfd> sockets;
		std::vector<Client> client_vector;
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
};