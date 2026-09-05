#include "signal.hpp"
#include "client.hpp"
#include "server.hpp"
#include <cstdlib>
#include <iostream>

int	main(int ac, char **av)
{
	try
	{
		if (ac != 3)
			throw std::runtime_error("Usage: ./ircserv <port> <password>");
		int port = std::atoi(av[1]);
		if (port < 1024 || port > 65535)
			throw std::runtime_error("Port number must be between 1024 and 65535");
		std::string password = av[2];
		if (password.empty())
			throw std::runtime_error("Password cannot be empty");
	
		setupSignals();
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
        return 1;
	}
	return (0);
}
    