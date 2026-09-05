#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include "client.hpp"
#include <sstream>

class Channel
{
private:
    std::string name;
    std::string topic;

    bool inviteOnly;
    bool topicRestricted;

    bool hasKey;
    std::string key;

    bool hasLimit;
    size_t limit;

    std::vector<Client*> users;
    std::vector<Client*> operators;
    std::vector<Client*> invitedUsers;

public:
    Channel();
    Channel(const std::string& channelName, Client* creator);
    std::string getName() const;
    void addUser(Client* target);
    void removeUser(int pos);
    void addInvitedUsers(Client* target);
    void addOperators(Client* target);
    size_t sizeOperators();
    // void sendMsg(Client& user, std::string &message);

    bool is_operator(int fd) const;
    bool is_invitedUser(int fd) const;
    void remove_invitedUser(int fd);
    void remove_operator(int fd);
    int plase_user_in_channel(int fd);
    bool is_inviteOnly() const;
    
    void setInviteOnly(bool auth);

    void setHasLimit(bool h);
    bool fullLimit() const;
    void setLimit(size_t l);

    bool is_hasKey() const;
    void setKey(bool hk, const std::string &k);
    std::string getKey();
    void removeKey();

    bool isTopicRestricted() const;
    void setTopicRestricted(bool t);
    void setTopic(const std::string &top);
    const std::string &getTopic() const;

    void broadcast(const std::string& msg);
    Client *getFirstUser();
    bool getHasLimit();
    std::string getNamesList();
    std::string getModesString() const;
    std::string getModesParams() const;
    void sendMsg(Client& user, std::string &message);
};