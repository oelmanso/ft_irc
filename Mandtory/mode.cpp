#include "server.hpp"

std::string Server::modeInvite(Client& user, Channel *channel, char sign)
{
    if (sign == '+')
    {
        channel->setInviteOnly(true);
        std::string msg = ":" + user.get_nickname() + " MODE " + channel->getName() + " +i\r\n";
        channel->broadcast(msg);
        return "";
    }
    channel->setInviteOnly(false);
    std::string msg = ":" + user.get_nickname() + " MODE " + channel->getName() + " -i\r\n";
    channel->broadcast(msg);
    return "";
}

std::string Server::modeTopic(Client& user, Channel *channel, char sign)
{
    if (sign == '+')
    {
        channel->setTopicRestricted(true);
        std::string msg = ":" + user.get_nickname() + " MODE " + channel->getName() + " +t\r\n";
        channel->broadcast(msg);
        return "";
    }
    channel->setTopicRestricted(false);
    std::string msg = ":" + user.get_nickname() + " MODE " + channel->getName() + " -t\r\n";
    channel->broadcast(msg);
    return "";
}

std::string Server::modeKey(Client& user, Channel *channel, char sign, const std::string &param)
{
    if (sign == '+')
    {
        if (channel->is_hasKey())
            return numericReply(467, user, channel->getName()
                + " :Channel key already set");
        channel->setKey(true, param);
        std::string msg = ":" + user.get_nickname() + " MODE " 
            + channel->getName() + " +k " + param + "\r\n";
        channel->broadcast(msg);
        return "";
    }
    channel->removeKey();
    std::string msg = ":" + user.get_nickname() + " MODE "
        + channel->getName() + " -k\r\n";
    channel->broadcast(msg);
    return "";
}
std::string Server::modeOperator(Client& user, Channel *channel, char sign, const std::string &param)
{
    Client *target = findUser(param);
    if (!target)
        return numericReply(401,user,
            param + " :No such nick");
    if (channel->plase_user_in_channel(target->get_fd()) == -1)
        return numericReply(441, user, param + " "
            + channel->getName() + " :They aren't on that channel");
    if (sign == '+')
    {
        if (channel->is_operator(target->get_fd()))
            return "";
        channel->addOperators(target);
        std::string msg = ":" + user.get_nickname() + " MODE "
               + channel->getName() + " +o "
               + target->get_nickname() + "\r\n";
        channel->broadcast(msg);
        return "";
    }
    if (!channel->is_operator(target->get_fd()))
        return "";
    channel->remove_operator(target->get_fd());
    if (channel->sizeOperators() == 0)
    {
        std::string msg =":" + user.get_nickname() + " MODE "
            + channel->getName() + " -o "
            + target->get_nickname() + "\r\n";
        channel->broadcast(msg);
        Client *tmp = channel->getFirstUser();
        if (tmp)
            channel->addOperators(tmp);
        msg = ":" + user.get_nickname() +
            " MODE " + channel->getName() +
            " +o " + tmp->get_nickname() + "\r\n";
        channel->broadcast(msg);
    }
    else
    {
        std::string msg =":" + user.get_nickname() + " MODE "
            + channel->getName() + " -o "
            + target->get_nickname() + "\r\n";
        channel->broadcast(msg);
    }
    return "";
}

std::string Server::modeLimit(Client& user, Channel *channel, char sign, const std::string &param)
{
    if (sign == '+')
    {
        if (!isNumber(param))
            return numericReply(461,user,
                "Invalid limit");
        size_t l = atoi(param.c_str());
        if (l <= 0)
            return numericReply(461,user,
                "Invalid limit");
        channel->setLimit(l);
        channel->setHasLimit(true);
        std::string msg = ":" + user.get_nickname() + " MODE "
            + channel->getName() + " +l " + param + "\r\n";
        channel->broadcast(msg);
        return "";
    }
    channel->setHasLimit(false);
    std::string msg = ":" + user.get_nickname() + " MODE "
        + channel->getName() + " -l\r\n";
    channel->broadcast(msg);
    return "";
}

bool Server::maxLimitPar(std::string &modes)
{
    char sign = '+';
    int paramModes = 0;

    for (size_t i = 0; i < modes.size(); i++)
    {
        if (modes[i] == '+' || modes[i] == '-')
        {
            sign = modes[i];
            continue;
        }
        if ((modes[i] == 'o') || 
            ((modes[i] == 'k' || modes[i] == 'l') && sign == '+'))
        {
            paramModes++;
            if (paramModes > 3)
                return true;
        }
    }
    return false;
}

std::string    Server::multiModes(Client& user, const std::vector<std::string>& args, Channel *channel)
{
    std::string modes = args[1];
    char sign = '+';
    size_t argIndex = 2;

    if (maxLimitPar(modes))
        return numericReply(461, user,
                    "Too many mode changes");
    for (size_t i = 0; i < modes.size(); i++)
    {
        if (modes[i] == '+' || modes[i] == '-')
        {
            sign = modes[i];
            continue;
        }
        std::string rep;
        if (modes[i] == 'i')
            rep = modeInvite(user, channel, sign);
        else if (modes[i] == 't')
            rep = modeTopic(user, channel, sign);
        else if (modes[i] == 'k')
        {
            if (sign == '+')
            {
                if (argIndex >= args.size())
                    return numericReply(461, user,
                        "MODE :Need key");
                rep = modeKey(user, channel, sign, args[argIndex]);
                argIndex++;
            }
            else
                rep = modeKey(user, channel, sign, "");
        }
        else if (modes[i] == 'l')
        {
            if (sign == '+')
            {
                if (argIndex >= args.size())
                    return numericReply(461, user,
                        "MODE :Need limit");
                rep = modeLimit(user, channel, sign, args[argIndex]);
                argIndex++;
            }
            else
                rep = modeLimit(user, channel, sign, "");
        }
        else if (modes[i] == 'o')
        {
            if (argIndex >= args.size())
                return numericReply(461, user,
                    "MODE :Need nickname");
            rep = modeOperator(user, channel, sign, args[argIndex]);
            argIndex++;
        }
        else
            return numericReply(472, user, modes + " :is unknown mode char to me");
        if (!rep.empty())
            return rep;
    }
    return "";
}

std::string Server::cmd_MODE(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user,
            "You have not registered");
    if (args.empty())
        return numericReply(461, user, 
            "MODE :Not enough parameters");
    if (args[0][0] != '#')
        return numericReply(476, user,
                args[0] + " :Bad Channel Mask");
    Channel *channel = findChannel(args[0]);
    if (!channel)
        return numericReply(403, user,
                args[0] + " :No such channel");
    if (channel->plase_user_in_channel(user.get_fd()) == -1)
        return numericReply(442, user,
                args[0] + " :You're not on that channel");
    if (args.size() == 1)
        return ":ircserv 324 " + user.get_nickname()
            + " " + channel->getName()
            + " " + channel->getModesString() 
            + channel->getModesParams() + "\r\n";
    if (!channel->is_operator(user.get_fd()))
        return numericReply(482, user,
        channel->getName() + " :You're not channel operator");
    return multiModes(user, args, channel);
}
