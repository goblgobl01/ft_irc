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

class Client {
	private:
		int client_socket;
		std::string buffer; 
	public:
		Client(int _client_socket);
		int get_client_fd();
};

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
};