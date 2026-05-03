#include "../../includes/Channel.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Server.hpp"


std::string Server::getClientPrefix(Client &client) {
	return (":" + client.get_nickname() + "!" + client.get_username() + "@localhost");
}

Channel *Server::findChannel(const std::string &name) {
	for (size_t i = 0; i < channel_vector.size(); i++)
	{
		if (channel_vector[i].getName() == name)
			return (&channel_vector[i]);
	}
	return (NULL);
}

Client  *Server::findClientByNick(const std::string &nickname)  {
	for (size_t i = 0; i < client_vector.size(); i++)
	{
		if (client_vector[i].get_nickname() == nickname)
			return (&client_vector[i]);
	}
	return (NULL);
}

void	Server::sendToClient(int fd, const std::string &message) {
    std::string msg = message + "\r\n";
    size_t total = 0;
    size_t len = msg.size();
    
    while (total < len)
    {
        int sent = send(fd, msg.c_str() + total, len - total, 0);
        if (sent == -1)
            return ;
        total += sent;
    }
}
