#include "server.hpp"

std::string Server::numericReply(int code, const Client& user,
const std::string& msg)
{
    std::ostringstream oss;
    oss << ":ircserv " << code << " ";
    if (!user.get_nickname().empty())
        oss << user.get_nickname();
    else
        oss << "*";
    oss << " :" << msg << "\r\n";
    return oss.str();
}
std::string Server::cmd_PASS(Client& user, const std::vector<std::string>& args)
{
    if (args.empty())
        return numericReply(461, user, "PASS :Not enough parameters");
    if (args.size() > 1)
        return numericReply(461, user, "PASS :Too many parameters");
    if (user.getRegistered())
        return numericReply(462, user, "You may not reregister");
    if (!user.get_nickname().empty() || !user.get_username().empty())
        return numericReply(462, user, "PASS must precede NICK/USER");
    if (args[0] == password)
    {
        user.setAuthenticated(true);
        checkRegistration(user);
        return "";
    }
    return numericReply(464, user, "Password incorrect");
}
std::string Server::cmd_NICK(Client& user, const std::vector<std::string>& args)
{
    if (args.empty())
        return numericReply(431, user, "No nickname given");
    if (args.size() > 1)
        return numericReply(461, user,
            "NICK :Too many parameters");
    if (!user.getAuthenticated())
        return numericReply(451, user, "You must send PASS first");
    if (!isValidNickname(args[0]))
        return numericReply(432, user, args[0] + " :Erroneous nickname");
    if (!checkNickname(args[0]))
        return numericReply(433, user, args[0] + " :Nickname is already in use");
    if (!user.get_nickname().empty())
    {
        std::string oldNick = user.get_nickname();
        user.set_nickname(args[0]);
        std::string msg = ":" + oldNick +
            " NICK :" + user.get_nickname() + "\r\n";
        send(user.get_fd(), msg.c_str(), msg.size(), 0);
        for (size_t i = 0; i < channels.size(); i++)
        {
            if (channels[i]->plase_user_in_channel(user.get_fd()) != -1)
                channels[i]->sendMsg(user, msg);
        }
    }
    else
        user.set_nickname(args[0]);
    checkRegistration(user);
    return "";
}

std::string Server::cmd_USER(Client& user, const std::vector<std::string>& args)
{
    if (args.size() < 4)
        return numericReply(461, user, "USER :Not enough parameters");
    if (user.getRegistered())
        return numericReply(462, user, "You may not reregister");
    if (!user.getAuthenticated())
        return numericReply(451, user, "You must send password first");
    if (user.get_nickname().empty())
        return numericReply(451, user, "You must send NICK first");
    if (args[3][0] != ':')
        return numericReply(461, user, "USER :Invalid format");
    std::string real = args[3].substr(1);
    if (real.empty())
        return numericReply(461, user, "USER :Not enough parameters");
    user.set_username(args[0]);
    user.set_realname(real);
    checkRegistration(user);
    return "";
}
