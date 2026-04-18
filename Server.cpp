#include "Server.hpp"

void	handle_command(std::string command)
{
	if (command == "JOIN")
		std::cout << "Hello From Join" << std::endl;
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

void Server::close_clients()
{
	for(size_t i = 0; i < client_vector.size(); i++)
	{
		std::cout << "Client <" << client_vector[i].get_client_fd() << "> Disconnected" << std::endl;
		close(client_vector[i].get_client_fd());
	}
	if (server_socket != -1)
	{
		std::cout << "Server <" << server_socket << "> Disconnected" << std::endl;
		close(server_socket);
	}
}

void Server::remove_a_client(Client &client)
{
	size_t i = 0;

	while (i < sockets.size())
	{
		if (sockets[i].fd == client.get_client_fd())
		{
			sockets.erase(sockets.begin() + i);
			break;
		}
		i++;
	}
	i = 0;
	while (i < client_vector.size())
	{
		if (client_vector[i].get_client_fd() == client.get_client_fd())
		{
			close(client.get_client_fd());
			client_vector.erase(client_vector.begin() + i);
			break ;
		}
		i++;
	}
}

std::string string_to_upper(std::string _string)
{
	for (size_t i = 0; i < _string.length(); ++i)
			_string[i] = std::toupper(_string[i]);
	return _string;
}

void Server::send_error(int client_fd, std::string code, std::string nickname, std::string command, std::string message)
{
	std::string response = ":localhost " + code + " " + nickname + " " + command + " :" + message + "\r\n";
	send(client_fd, response.c_str(), response.length(), 0);
}

void Server::parse_buffer(Client &client)
{
	std::string &buf = client.get_buffer();
	size_t pos;

	while ((pos = buf.find("\r\n")) != std::string::npos)
	{
		std::string line = buf.substr(0, pos);
		buf.erase(0, pos + 2);
		if (line.empty())
			continue;
		std::stringstream ss(line);
		std::string command;
		std::string content;

		ss >> command;
		command = string_to_upper(command);
		if (command == "PASS")
		{
			if (!(ss >> content))
				send_error(client.get_client_fd(), "461", "*", "PASS", "password cant be empty");
			else if (client.pass_status() == true)
				send_error(client.get_client_fd(), "462", "*", "PASS", "already connected");
			else if (content == this->passwd)
				client.set_pass(true);
			else
			{
				// send_error(client.get_client_fd(), "464", "*", "PASS", "pass_missmatch");
				remove_a_client(client);
				return ;
			}
		}
		else if (command == "NICK")
		{
			if (!client.pass_status())
			{
				send_error(client.get_client_fd(), "451", "*", "NICK", "not authenticated yet");
				continue ;
			}
			else if (!(ss >> content))
			{
				send_error(client.get_client_fd(), "431", "*", "NICK", "No nickname given");
				continue ;
			}
			bool found_duplicate = false;
			for (size_t i = 0; i < client_vector.size(); ++i)
			{
				if (client_vector[i].get_client_fd() != client.get_client_fd())
				{
					if (client_vector[i].nick_status())
						if (string_to_upper(client_vector[i].get_nickname()) == string_to_upper(content))
						{
							found_duplicate = true;
							break ;
						}
				}
			}
			if (!found_duplicate)
			{
				client.set_nick(true);
				client.set_nickname(content);
			}
			else
				send_error(client.get_client_fd(), "433", "*", content, "Nickname is already in use");
			if (client.is_registered())
			{
				std::string welcome = ":localhost 001 " + client.get_nickname() + " :Welcome to the Internet Relay Network " + client.get_nickname() + "\r\n";
				send(client.get_client_fd(), welcome.c_str(), welcome.length(), 0);
			}
		}
		else if (command == "USER")
		{
			if (!client.pass_status())
			{
				send_error(client.get_client_fd(), "451", "*", "USER", "You have not registered");
				continue;
			}
			if (client.user_status())
			{
				send_error(client.get_client_fd(), "462", client.get_nickname(), "USER", "Unauthorized command (already registered)");
				continue;
			}
			std::string user, host, server, real;
			if (!(ss >> user >> host >> server))
			{
				send_error(client.get_client_fd(), "461", client.get_nickname().empty() ? "*" : client.get_nickname(), "USER", "Not enough parameters");
				continue;
			}
			std::getline(ss, real);
			if (real.empty() || real.find_first_not_of(" \t\r\n") == std::string::npos)
			{
				send_error(client.get_client_fd(), "461", client.get_nickname().empty() ? "*" : client.get_nickname(), "USER", "Not enough parameters");
				continue;
			}
			size_t colon_pos = real.find(':');
			if (colon_pos != std::string::npos)
				real = real.substr(colon_pos + 1);
			else
			{
				size_t first = real.find_first_not_of(" ");
				if (first != std::string::npos)
					real = real.substr(first);
			}
			client.set_username(user);
			client.set_realname(real);
			client.set_user(true);
			if (client.is_registered())
			{
				std::string welcome = ":localhost 001 " + client.get_nickname() + " :Welcome to our Internet Relay Network " + client.get_nickname() + "\r\n";
				send(client.get_client_fd(), welcome.c_str(), welcome.length(), 0);
			}
		}
	}
}

void Server::receive_new_data(int i)
{
	char buffer[4096];
	size_t n = 0;

	memset(buffer, 0, 4096);
	int bytesReceived = recv(sockets[i].fd, buffer, 4096, 0);
	std::cout << buffer <<std::endl;
	if (bytesReceived <= 0)
	{
		std::cout << sockets[i].fd << "Client disconnected" << std::endl;
		close(sockets[i].fd);
		sockets.erase(sockets.begin() + i);
		client_vector.erase(client_vector.begin() + (i - 1));
	}
	else
	{
		while(n < client_vector.size())
		{
			if (client_vector[n].get_client_fd() == sockets[i].fd)
			{
				this->client_vector[n].set_buffer(buffer);
				parse_buffer(client_vector[n]);
			}
			n++;
		}
	}
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
				{
					receive_new_data(i);
				}
			}
			i++;
		}
	}
}