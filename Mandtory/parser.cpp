#include "server.hpp"

Command parse(std::string msg)
{
    while (!msg.empty() && (msg[msg.size() - 1] == '\r'
        || msg[msg.size() - 1] == '\n'))
        msg.erase(msg.size() - 1);
    
    Command cmd;
    std::istringstream ss(msg);
    ss >> cmd.name;
    std::string word;
    while (ss >> word)
    {
        if (!word.empty() && word[0] == ':')
        {
            std::string rest;
            std::getline(ss, rest);
            if (!checkWatespaces(rest))
                word += rest;
            cmd.args.push_back(word);
            break;
        }
        cmd.args.push_back(word);
    }
    return cmd;
}

std::string Server::handleCommand(Client& user, const std::string& msg)
{
    Command cmd = parse(msg);

    if (cmd.name.empty())
        return "";
    if (cmd.name == "NICK")
        return cmd_NICK(user, cmd.args);
    if (cmd.name == "USER")
        return cmd_USER(user, cmd.args);
    if (cmd.name == "PASS")
        return cmd_PASS(user, cmd.args);
    if (cmd.name == "JOIN")
        return cmd_JOIN(user, cmd.args);
    if (cmd.name == "PRIVMSG")
        return cmd_PRIVMSG(user, cmd.args);
    if (cmd.name == "TOPIC")
        return cmd_TOPIC(user, cmd.args);
    if (cmd.name == "INVITE")
        return cmd_INVITE(user, cmd.args);
    if (cmd.name == "KICK")
        return cmd_KICK(user, cmd.args);
    if (cmd.name == "MODE")
        return cmd_MODE(user, cmd.args);
    if (cmd.name == "CAP")
        return "";
    if (cmd.name == "WHO")
        return "";
    return "ERROR: unknown command\r\n";
}
