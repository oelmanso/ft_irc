#include "server.hpp"

std::string Server::cmd_PRIVMSG(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user, ":You have not registered");
    if (args.empty())
        return numericReply(411, user,
                ":No recipient given (PRIVMSG)");
    if (args.size() < 2)
        return numericReply(412, user,
                ":No text to send");
    if (args[1][0] != ':')
        return numericReply(412, user,
                ":Invalid message format");
    std::string text = args[1].substr(1);
    if (text.empty())
        return numericReply(412, user,
                ":No text to send");
    std::string message = ":" + user.get_nickname() + " PRIVMSG "
        + args[0] + " :" + text + "\r\n";
    if (args[0][0] != '#')
    {
        Client *target = findUser(args[0]);
        if (!target)
            return numericReply(401, user,
                    args[0] + " :No such nick");
        send(target->get_fd(), message.c_str(), message.size(), 0);
        return "";
    }
    Channel *channel = findChannel(args[0]);
    if (!channel)
        return numericReply(401, user,
                    args[0] + " :No such nick");
    if (channel->plase_user_in_channel(user.get_fd()) == -1)
        return numericReply(404, user,
                args[0] + " :Cannot send to channel");
    channel->broadcast(message);
    return "";
}
std::string Server::cmd_TOPIC(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user, ":You have not registered");
    if (args.empty())
        return numericReply(461, user, "TOPIC :Not enough parameters");
    if (args[0][0] != '#')
        return numericReply(476, user, args[0] + ":Bad Channel Mask");
    Channel *channel = findChannel(args[0]);
    if (!channel)
        return numericReply(403, user, args[0] + " :No such channel");
    if (channel->plase_user_in_channel(user.get_fd()) == -1)
        return numericReply(442, user, args[0] + " :You're not on that channel");
    if (args.size() == 1)
    {
        if (channel->getTopic().empty())
            return numericReply(331, user, args[0] + ":No topic is set");
        return numericReply(332, user, args[0] + " :" + channel->getTopic());
    }
    if (channel->isTopicRestricted() && !channel->is_operator(user.get_fd()))
        return numericReply(482, user, args[0] + ":You're not channel operator");
    if (args[1][0] != ':')
        return "ERROR:  invalid topic format\r\n";
    std::string topic = args[1].substr(1);
    channel->setTopic(topic);
    std::string msg = ":" + user.get_nickname() + " TOPIC " 
        + channel->getName() + " :" + topic + "\r\n";
    channel->broadcast(msg);
    return "";
}

std::string Server::cmd_INVITE(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user, ":You have not registered");
    if (args.size() < 2)
        return numericReply(461, user, "INVITE :Not enough parameters");
    if (args.size() > 2)
        return numericReply(461, user, "INVITE :Too many parameters");
    if (args[1][0] != '#')
        return numericReply(476, user, args[1] + ":Bad Channel Mask");
    Channel *channel = findChannel(args[1]);
    if (!channel)
         return numericReply(403, user, args[1] + ":No such channel");
    if (channel->plase_user_in_channel(user.get_fd()) == -1)
        return numericReply(442, user, args[1] + ":You're not on that channel");
    if (!channel->is_operator(user.get_fd()))
        return numericReply(482, user, args[1] + ":You're not channel operator");
    Client *target = findUser(args[0]);
    if (!target)
        return numericReply(401, user, args[0] + ":No such nick");
    if (channel->plase_user_in_channel(target->get_fd()) != -1)
        return numericReply(443, user, target->get_nickname()
            + " " + channel->getName() + ":is already on channel");
    if (channel->is_invitedUser(target->get_fd()))
        return "ERROR: user already invited\r\n";
    channel->addInvitedUsers(target);
    std::string reply = ":ircserv 341 " + user.get_nickname() + " "
        + target->get_nickname() + " " + channel->getName()
        + "\r\n";
    send(user.get_fd(), reply.c_str(), reply.size(), 0);
    std::string msg = ":" + user.get_nickname() + " INVITE " 
        + target->get_nickname() + " " + channel->getName() + "\r\n";
    send(target->get_fd(), msg.c_str(), msg.size(), 0);
    return "";
}

std::string Server::cmd_KICK(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user,
                ":You have not registered");
    if (args.size() < 2)
        return numericReply(461, user,
                "KICK :Not enough parameters");
    if (args[0][0] != '#')
        return numericReply(476, user,
                args[0] + ":Bad Channel Mask");
    Channel *channel = findChannel(args[0]);
    if (!channel)
        return numericReply(403, user,
                args[0] + ":No such channel");
    if (channel->plase_user_in_channel(user.get_fd()) == -1)
        return numericReply(442, user,
                args[0] + ":You're not on that channel");
    if (!channel->is_operator(user.get_fd()))
         return numericReply(482, user,
                args[0] + ":You're not channel operator");
    Client *target = findUser(args[1]);
    if (!target)
        return numericReply(401, user,
                args[1] + ":No such nick");
    if (channel->plase_user_in_channel(target->get_fd()) == -1)
        return numericReply(441, user, target->get_nickname() + " " 
            + channel->getName() + ":They aren't on that channel");
    std::string msg = ":" + user.get_nickname() + " KICK " + channel->getName() + " " + target->get_nickname();
    
    if (args.size() >= 3)
    {
        if (args[2][0] != ':')
            return "ERROR: invalid reason format\r\n";
        msg += " " + args[2];
    }
    msg += "\r\n";
    channel->broadcast(msg);
    bool wasOperator = channel->is_operator(target->get_fd());
    if (wasOperator)
        channel->remove_operator(target->get_fd());
    int pos = channel->plase_user_in_channel(target->get_fd());
    channel->removeUser(pos);
    if (wasOperator && channel->sizeOperators() == 0)
    {
        Client *newOp = channel->getFirstUser();
        if (newOp)
            channel->addOperators(newOp);
        std::string msg = ":" + user.get_nickname() +
            " MODE " + channel->getName() +
            " +o " + newOp->get_nickname() + "\r\n";
        channel->broadcast(msg);
    }
    return "";
}
