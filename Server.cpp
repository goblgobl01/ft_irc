#include "Server.hpp"

Client::Client(int _client_socket) : client_socket(_client_socket) {}
int Client::get_client_fd()
{
	return (client_socket);
}

Server::Server(std::string port_str, std::string _passwd) : passwd(_passwd) {
	if (port_str.empty() || port_str.find_first_not_of("0123456789") != std::string::npos)
		throw std::runtime_error("Error: Invalid port characters");
	std::stringstream ss(port_str);
	long p;
	if (!(ss >> p))
		throw std::runtime_error("Error: Port conversion failed");
	if (p < 1024 || p > 65535)
		throw std::runtime_error("Error: Port out of range (1024-65535)");
	this->port = static_cast<int>(p);
}

void Server::server_init()
{
	struct sockaddr_in	hint;
	struct pollfd		serverpollfd;
	int					option_value;

	this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->server_socket == -1)
		throw std::runtime_error("Error: SOCKET system call failed");
	option_value = 1;
	if(setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) == -1) //-> set the socket option (SO_REUSEADDR) to reuse the address
		throw(std::runtime_error("faild to set option (SO_REUSEADDR) on socket"));
	if (fcntl(this->server_socket, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Error: FCNTL system call failed server");
	hint.sin_family = AF_INET;
	hint.sin_port = htons(this->port);
	inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
	if (bind(this->server_socket, (struct sockaddr *)&hint, sizeof(hint)) == -1)
		throw std::runtime_error("Error: FCNTL system call failed");
	if (listen(this->server_socket, SOMAXCONN) == -1)
		throw std::runtime_error("Error: listen failed");
	serverpollfd.fd = this->server_socket;
	serverpollfd.events = POLLIN;
	serverpollfd.revents = 0;
	sockets.push_back(serverpollfd);
}

void Server::accept_new_client()
{
	int				new_client_socket;
	sockaddr_in		client;
	socklen_t		clientSize = sizeof(client);
	struct pollfd	clientpollfd;

	if ((new_client_socket = accept(this->server_socket, (struct sockaddr *)&client, &clientSize)) == -1)
		throw std::runtime_error("Error: accepting new client failed");
	if (fcntl(new_client_socket, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Error: FCNTL system call failed client");
	clientpollfd.fd = new_client_socket;
	clientpollfd.events = POLLIN;
	clientpollfd.revents = 0;
	Client new_client(new_client_socket);
	this->sockets.push_back(clientpollfd);
	this->client_vector.push_back(new_client);
	std::cout << new_client.get_client_fd() << "Client connected" << std::endl;
}

void Server::receive_new_data(int i)
{
	char buffer[4096];
	memset(buffer, 0, 4096);
	int bytesReceived = recv(sockets[i].fd, buffer, 4096, 0);
	if (bytesReceived == -1)
		throw std::runtime_error("Error: recv failed");
	if (bytesReceived == 0)
	{
		std::cout << sockets[i].fd << "Client disconnected" << std::endl;
		close(sockets[i].fd);
		sockets.erase(sockets.begin() + i);
		client_vector.erase(client_vector.begin() + (i - 1));
	}
	else
		std::cout << client_vector[i - 1].get_client_fd() << "client: " << std::string(buffer, 0, bytesReceived) << std::endl;
}



void Server::main_loop()
{
	while(true)
	{
		if (poll(&this->sockets[0], sockets.size(), -1) == -1)
			throw std::runtime_error("Error: poll failed");
		size_t i = 0;
		while(i < sockets.size())
		{
			if (sockets[i].revents & POLLIN)
			{
				if (sockets[i].fd == this->server_socket)
					accept_new_client();
				else
					receive_new_data(i);
			}
			i++;
		}
	}
}