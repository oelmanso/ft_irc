#include "server.hpp"

bool Server::checkNickname(const std::string& nick)
{
    std::map<int, Client*>::iterator it = clients.begin();

    for (; it != clients.end(); ++it)
    {
        if (it->second->get_nickname() == nick)
            return false;
    }
    return true;
}
void Server::checkRegistration(Client& client)
{
    if (client.getRegistered())
        return;

    if (client.getAuthenticated()
        && !client.get_nickname().empty()
        && !client.get_username().empty())
    {
        client.setRegistered(true);
        std::string msg = ":ircserv 001 " + client.get_nickname() +
            " :Welcome to the IRC server\r\n";
        send(client.get_fd(), msg.c_str(), msg.size(), 0);
    }
}

bool Server::isValidNickname(const std::string& nick)
{
    if (nick.empty())
        return false;
    if (!std::isalpha((unsigned char)nick[0]) &&
        nick[0] != '_' && nick[0] != '[' && nick[0] != '{')
        return false;
    for (size_t i = 0; i < nick.size(); i++)
    {
        char c = nick[i];

        if (std::isalnum((unsigned char)c) || c == '_' || c == '-' ||
            c == '[' || c == ']' || c == '{' || c == '}' ||
            c == '\\' || c == '`' || c == '^')
            continue;
        return false;
    }
    return true;
}
bool Server::isNumber(const std::string& num)
{
    if (num.empty())
        return false;
    for (size_t i = 0; i < num.size(); i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(num[i])))
            return false;
    }
    return true;
}
bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    for (size_t i = 0; i < a.size(); i++)
    {
        if (std::tolower(a[i]) != std::tolower(b[i]))
            return false;
    }
    return true;
}
Channel* Server::findChannel(const std::string& name)
{
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (equalsIgnoreCase(channels[i]->getName(), name))
            return channels[i];
    }
    return NULL;
}
Client *Server::findUser(const std::string& name)
{
    std::map<int, Client *>::iterator it = clients.begin();
    for (; it != clients.end(); it++)
    {
        if (it->second->get_nickname() == name)
            return it->second;
    }
    return NULL;
}
bool checkWatespaces(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        if (!std::isspace(static_cast<unsigned char>(str[i])))
            return false;
    }
    return true;
}
