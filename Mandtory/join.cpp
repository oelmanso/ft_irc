#include "server.hpp"

void Server::sendNamesList(Client& user, Channel* channel)
{
    std::string names = ":ircserv 353 " + user.get_nickname()
        + " = " + channel->getName() + " :"
        + channel->getNamesList() + "\r\n";
    send(user.get_fd(), names.c_str(), names.size(), 0);
    std::string end = ":ircserv 366 " + user.get_nickname()
        + " " + channel->getName() + " :End of /NAMES list\r\n";
    send(user.get_fd(), end.c_str(), end.size(), 0);
}

std::string Server::createChannel(Client& user, const std::string& channelName)
{
    Channel *obj = new Channel(channelName, &user);
        channels.push_back(obj);
    std::string msg = ":" + user.get_nickname() + " JOIN "
        + obj->getName() + "\r\n";
    obj->broadcast(msg);
    std::string reply;
    reply = numericReply(331, user,
            channelName + " :No topic is set");
    send(user.get_fd(), reply.c_str(), reply.size(), 0);
    sendNamesList(user, obj);
    return "";
}

std::string Server::joinOneChannel(Client& user, const std::string& channelName, const std::string& key)
{
    if (channelName.empty())
        return numericReply(403, user, channelName + " :No such channel");
    if (channelName[0] != '#')
        return numericReply(476, user, channelName + " :Bad Channel Mask");
    Channel *channel = findChannel(channelName);
    if (!channel)
        return createChannel(user, channelName);
    if (channel->plase_user_in_channel(user.get_fd()) != -1)
        return "ERROR: already in channel\r\n";
    if (channel->is_hasKey())
    {
        if (key.empty())
            return numericReply(475, user,
                channelName + " :Cannot join channel (+k)");        
        if (key != channel->getKey())
            return numericReply(475, user, channelName 
                + " :bad key (+k)");
    }
    if (channel->fullLimit())
        return numericReply(471, user,
                channelName + " :Cannot join channel (+l)");
    if (channel->is_inviteOnly())
    {
        if (!channel->is_invitedUser(user.get_fd()))
            return numericReply(473, user, 
                channelName + " :Cannot join channel (+i)");
        channel->remove_invitedUser(user.get_fd());
    }
    channel->addUser(&user);
    std::string msg = ":" + user.get_nickname()
        + " JOIN " + channel->getName() + "\r\n";
    channel->broadcast(msg);
    std::string reply;
    if (channel->getTopic().empty())
        reply = numericReply(331, user,
            channelName + " :No topic is set");
    else
        reply = numericReply(332, user,
            channelName + " :" + channel->getTopic());
    send(user.get_fd(), reply.c_str(), reply.size(), 0);
    sendNamesList(user, channel);
    return "";
}

std::string Server::cmd_JOIN(Client& user, const std::vector<std::string>& args)
{
    if (!user.getRegistered())
        return numericReply(451, user, "You have not registered");
    if (args.empty())
        return numericReply(461, user, "JOIN :Not enough parameters");
    if (args.size() > 2)
        return numericReply(461, user, "JOIN :Too many parameters");
    std::vector<std::string> chanls;
    std::vector<std::string> keys;
    std::stringstream ss(args[0]);
    std::string token;
    while (std::getline(ss, token, ','))
        chanls.push_back(token);
    if (args.size() == 2)
    {
        std::stringstream ss2(args[1]);
        while (std::getline(ss2, token, ','))
            keys.push_back(token);
    }
    for (size_t i = 0; i < chanls.size(); i++)
    {
        std::string key;
        if (i < keys.size())
            key = keys[i];
        std::string reply = joinOneChannel(user, chanls[i], key);
        if (!reply.empty())
            send(user.get_fd(), reply.c_str(), reply.size(), 0);
    }
    return "";
}
