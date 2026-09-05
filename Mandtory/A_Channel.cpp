#include "A_Channel.hpp"

Channel::Channel()
    : inviteOnly(false),
      topicRestricted(false),
      hasKey(false),
      hasLimit(false),
      limit(0)
{
}
Channel::Channel(const std::string& channelName, Client* creator)
    : name(channelName),
      inviteOnly(false),
      topicRestricted(false),
      hasKey(false),
      hasLimit(false),
      limit(0)
{
    users.push_back(creator);
    operators.push_back(creator);
}
void Channel::addUser(Client* target)
{
    if (plase_user_in_channel(target->get_fd()) == -1)
        users.push_back(target);
}
Client *Channel::getFirstUser()
{
    if (users.empty())
        return NULL;
    return users[0];
}

std::string Channel::getName() const
{
    return name;
}

void Channel::removeUser(int pos)
{
    users.erase(users.begin() + pos);
}
void Channel::addInvitedUsers(Client* target)
{
    invitedUsers.push_back(target);
}
void Channel::addOperators(Client* target)
{
    operators.push_back(target);
}
size_t Channel::sizeOperators()
{
    return (operators.size());
}
void Channel::sendMsg(Client& user, std::string &message)
{
    for (size_t i = 0; i < users.size(); i++)
    {
        if (users[i]->get_fd() != user.get_fd())
            send(users[i]->get_fd(), message.c_str(), message.size(), 0);
    }
}
bool Channel::is_operator(int fd) const
{
    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i]->get_fd() == fd)
            return true;
    }
    return false;
}

void Channel::remove_invitedUser(int fd)
{
    for (size_t i = 0; i < invitedUsers.size(); i++)
    {
        if (invitedUsers[i]->get_fd() == fd)
        {
            invitedUsers.erase(invitedUsers.begin() + i);
            return ;
        }
    }
    
}
void Channel::remove_operator(int fd)
{
    for (size_t i = 0; i < operators.size(); i++)
    {
        if (operators[i]->get_fd() == fd)
        {
            operators.erase(operators.begin() + i);
            return;
        }
    }
}
bool Channel::is_invitedUser(int fd) const
{
    for (size_t i = 0; i < invitedUsers.size(); i++)
    {
        if (invitedUsers[i]->get_fd() == fd)
            return true;
    }
    return false;
}
int Channel::plase_user_in_channel(int fd)
{
    for (size_t i = 0; i < users.size(); i++)
    {
        if (users[i]->get_fd() == fd)
            return i;
    }
    return -1;
}

bool Channel::is_inviteOnly() const
{
    return inviteOnly;
}

void Channel::setInviteOnly(bool auth)
{
    inviteOnly = auth;
}
void Channel::setHasLimit(bool h)
{
    hasLimit = h;
}
bool Channel::getHasLimit()
{
    return hasLimit;
}
bool Channel::fullLimit() const
{
    if (hasLimit && users.size() >= limit)
        return true; 
    return false;
}
void Channel::setLimit(size_t l)
{
    limit = l;
}
bool Channel::is_hasKey() const
{
    return hasKey;
}
void Channel::setKey(bool keyEnabled, const std::string &newkey)
{
    hasKey = keyEnabled;
    key = newkey;
}
std::string Channel::getKey()
{
    return key;
}
void Channel::removeKey()
{
    hasKey = false;
    key.clear();
}
bool Channel::isTopicRestricted() const
{
    return topicRestricted;
}
void Channel::setTopicRestricted(bool t)
{
    topicRestricted = t;
}
void Channel::setTopic(const std::string &top)
{
    topic = top;
}
const std::string &Channel::getTopic() const
{
    return topic;
}
void Channel::broadcast(const std::string& msg)
{
    for (size_t i = 0; i < users.size(); i++)
        send(users[i]->get_fd(), msg.c_str(), msg.size(), 0);
}

std::string Channel::getNamesList()
{
    std::string names;

    for (size_t i = 0; i < users.size(); i++)
    {
        if (is_operator(users[i]->get_fd()))
            names += "@";

        names += users[i]->get_nickname();

        if (i + 1 < users.size())
            names += " ";
    }
    return names;
}
std::string Channel::getModesString() const
{
    std::string modes = "+";

    if (inviteOnly)
        modes += "i";
    if (topicRestricted)
        modes += "t";
    if (hasKey)
        modes += "k";
    if (hasLimit)
        modes += "l";
    return modes;
}
std::string Channel::getModesParams() const
{
    std::string params;

    if (hasKey)
        params += " " + key;

    if (hasLimit)
    {
        std::ostringstream oss;//??
        oss << limit;
        params += " " + oss.str();
    }
    return params;
}
