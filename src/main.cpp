#include "../includes/Server.hpp"

/*
check a client is fully registered (completed PASS + NICK + USER)
Client (a connected user)

client.get_client_fd(); => the socket fd, used to send msg to THIS client
client.get_nickname(); => used in IRC messages and error replies
client.get_username(); => used in the prefix :nick!user@localhost
client.is_registered(); => true only when PASS + NICK + USER all done

always check if the client is regitered first
always check if the channel exists
always check if the client is in the channel before doing anything
and for operator commands, check if it is an operator

*/

int main(int ac, char **av)
{
	if (ac != 3)
		return 1;
	try
	{
		Server myServer(av[1], av[2]);
		myServer.server_init();
		myServer.main_loop();
		myServer.close_clients();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}