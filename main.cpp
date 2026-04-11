#include "Server.hpp"

int main(int ac, char **av)
{
	if (ac != 3)
		return 1;
	try
	{
		Server myServer(av[1], av[2]);
		myServer.server_init();
		myServer.main_loop();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}