#include "Server.hpp"


// Bot

std::vector<Client>& Server::getClients() 
{
	return client_vector;
}

std::vector<Channel>& Server::getChannels() 
{
	return channel_vector;
}

// end bot

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
		throw std::runtime_error("Error: BIND system call failed");
	if (listen(this->server_socket, SOMAXCONN) == -1)
		throw std::runtime_error("Error: listen failed");
	serverpollfd.fd = this->server_socket;
	serverpollfd.events = POLLIN;
	serverpollfd.revents = 0;
	sockets.push_back(serverpollfd);
	client_vector.reserve(100);
	channel_vector.reserve(100);
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
	sendToClient(client_fd, response);
}

int check_special_characters(const std::string& str)
{
	std::string allowed_symbols = "-[]\\`^{}|_";

	for (std::string::size_type i = 0; i < str.length(); ++i) {
		unsigned char c = static_cast<unsigned char>(str[i]);
		if (std::isalnum(c)) {
			continue;
		}
		if (allowed_symbols.find(str[i]) != std::string::npos) {
			continue;
		}
		return 1;
	}
	return 0;
}

void Server::parse_buffer(Client &client)
{
	std::string &buf = client.get_buffer();
	size_t pos = 0;

	while ((pos = buf.find('\n')) != std::string::npos)
	{
		std::string line = buf.substr(0, pos);
		buf.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
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
			{
				client.set_pass(true);
			}
			else
			{
				send_error(client.get_client_fd(), "464", "*", "PASS", "wrong passwd");
				continue ;
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
			if (isdigit(content[0]) || content[0] == '~' || content.length() > 9)
			{
				send_error(client.get_client_fd(), "432", content, "NICK", "nickname error");
				continue ;
			}
			if (check_special_characters(content))
			{
				std::cout << content << std::endl;
				send_error(client.get_client_fd(), "432", content, "NICK", "nickname error");
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
				std::string old_nick = client.get_nickname();
				std::string new_nick = content;
				std::string prefix = ":" + old_nick + "!" + client.get_username() + "@localhost";
				std::string broadcast_msg = prefix + " NICK :" + new_nick + "\r\n";
				client.set_nick(true);
				client.set_nickname(new_nick);
				if (client.is_registered())
				{
					sendToClient(client.get_client_fd(), broadcast_msg);
					for (size_t i = 0; i < channel_vector.size(); ++i)
					{
						if (channel_vector[i].isMember(&client))
							channel_vector[i].broadcastMessage(broadcast_msg, &client);
					}
				}
			}
			else
				send_error(client.get_client_fd(), "433", "*", content, "Nickname is already in use");
			if (client.is_registered() && client.auth_status() == false)
			{
				std::string nick = client.get_nickname();
				std::string rpl;
				rpl = ":localhost 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 002 " + nick + " :Your host is localhost, running version 1.0\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 003 " + nick + " :This server was created May 09 2026\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 004 " + nick + " localhost 1.0 i tkol\r\n";
				sendToClient(client.get_client_fd(), rpl);
				client.set_auth(true);
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
			size_t first_non_space = real.find_first_not_of(" \t");
			if (first_non_space == std::string::npos)
			{
				send_error(client.get_client_fd(), "461", client.get_nickname().empty() ? "*" : client.get_nickname(), "USER", "Not enough parameters");
				continue;
			}
			std::string trailing = real.substr(first_non_space);
			if (trailing[0] != ':')
			{
				send_error(client.get_client_fd(), "461", client.get_nickname().empty() ? "*" : client.get_nickname(), "USER", "Real name must start with ':'");
				continue;
			}
			std::string final_real_name = trailing.substr(1);
			if (final_real_name.empty())
			{
				send_error(client.get_client_fd(), "461", client.get_nickname().empty() ? "*" : client.get_nickname(), "USER", "Real name cannot be empty");
				continue;
			}
			client.set_username(user);
			client.set_realname(final_real_name);
			client.set_user(true);
			if (client.is_registered() && client.auth_status() == false)
			{
				std::string nick = client.get_nickname();
				std::string rpl;
				rpl = ":localhost 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 002 " + nick + " :Your host is localhost, running version 1.0\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 003 " + nick + " :This server was created May 09 2026\r\n";
				sendToClient(client.get_client_fd(), rpl);
				rpl = ":localhost 004 " + nick + " localhost 1.0 i tkol\r\n";
				sendToClient(client.get_client_fd(), rpl);
				client.set_auth(true);
			}
		}
		else
			handleCommands(client, command, ss);
	}
}

void Server::receive_new_data(int i)
{
	char buffer[4096];
	size_t n = 0;

	memset(buffer, 0, 4096);
	int bytesReceived = recv(sockets[i].fd, buffer, 4096, 0);
	if (bytesReceived <= 0)
	{
		std::cout << sockets[i].fd << "Client disconnected" << std::endl;
		for (size_t n = 0; n < client_vector.size(); n++)
		{
			if (client_vector[n].get_client_fd() == sockets[i].fd)
			{
				removeClientFromAllChannels(client_vector[n], "Connection reset");
				remove_a_client(client_vector[n]);
				break ;
			}
		}
		return;
	}
	else
	{
		while(n < client_vector.size())
		{
			if (client_vector[n].get_client_fd() == sockets[i].fd)
			{
				this->client_vector[n].set_buffer(std::string(buffer, bytesReceived));
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
