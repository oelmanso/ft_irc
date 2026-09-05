#include "Bot.hpp"
#include <iostream>
#include <cstdlib>

int main(int ac, char **av)
{
    try
    {
        if (ac != 5 && ac != 6)
        {
            std::cerr << "Usage: ./bot <host> <port> <password> <channel> [nickname]" << std::endl;
            std::cerr << "Example: ./bot 127.0.0.1 6667 secret \"#chat\" mybot" << std::endl;
            return 1;
        }
        std::string host = av[1];
        int port = std::atoi(av[2]);
        if (port < 1 || port > 65535)
        {
            std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
            return 1;
        }
        std::string password = av[3];
        std::string channel = av[4];
        if (channel[0] != '#')
            channel = "#" + channel;
        std::string nickname = "BotBot";
        if (ac == 6)
            nickname = av[5];
        std::srand(std::time(NULL));
        Bot bot(host, port, password, channel, nickname);
        bot.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Bot error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
