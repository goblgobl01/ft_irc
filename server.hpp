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

class Server {
	private:
		int port;
		int server_socket;
		std::string passwd
		std::vector<struct pollfd> *sockets;
	public:
		Server(std::string port, std::string passwd);
		void server_init();
};