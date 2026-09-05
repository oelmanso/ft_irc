#pragma once


#include "client.hpp"
#include "A_Channel.hpp"
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>  //inet_ntoa()

#include <sstream>
#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <cstdio>


struct Command 
{
    std::string name;
    std::vector<std::string> args;
};

class Server
{
  private:
	int server_fd;
	sockaddr_in serverAddress;
	int port;
	std::string password;
	std::map<int, Client *> clients;
	std::vector<pollfd> poll_fds;
	std::vector<Channel*> channels;


	std::string cmd_PASS(Client& user, const std::vector<std::string>& args);
    std::string cmd_NICK(Client& user, const std::vector<std::string>& args);
    std::string cmd_USER(Client& user, const std::vector<std::string>& args);
    std::string cmd_JOIN(Client& user, const std::vector<std::string>& args);
    std::string cmd_PRIVMSG(Client& user, const std::vector<std::string>& args);
    std::string cmd_TOPIC(Client& user, const std::vector<std::string>& args);
    std::string cmd_INVITE(Client& user, const std::vector<std::string>& args);
    std::string cmd_KICK(Client& user, const std::vector<std::string>& args);
    std::string cmd_MODE(Client& user, const std::vector<std::string>& args);
	Channel* findChannel(const std::string& name);
    Client* findUser(const std::string& name);
	void removeClientFromChannels(int client_fd);
	
  public:
	Server();
	Server(int port, const std::string &password);
    ~Server();
	void run();
	void create_socket();
    void setsockpt();
	void bind_socket();
	void listen_socket();
	void accept_client();
	void receive_data(int client_fd);
	void disconnect_client(int client_fd);
    void sendToClient(int client_fd, const std::string &msg);
	void setNonBlocking(int fd);
	void updatePollEvents(int client_fd);
	void sendPendingData(int client_fd);
	std::string handleCommand(Client& user, const std::string& msg);
	void checkRegistration(Client& user);
	bool checkNickname(const std::string& nick);
	bool isValidNickname(const std::string& nick);
	bool isNumber(const std::string& num);
	std::string modeInvite(Client& user, Channel *channel, char sign);
	std::string modeTopic(Client& user, Channel *channel, char sign);
	std::string modeKey(Client& user, Channel *channel, char sign , const std::string &param);
	std::string modeOperator(Client& user, Channel *channel, char sign , const std::string &param);
	std::string modeLimit(Client& user, Channel *channel, char sign , const std::string &param);
	std::string multiModes(Client& user, const std::vector<std::string>& args, Channel *channel);
	bool maxLimitPar(std::string &modes);
	std::string joinOneChannel(Client& user, const std::string& channelName, const std::string& key);
	std::string createChannel(Client& user, const std::string& channelName);
	void sendNamesList(Client& user, Channel* channel);
	std::string numericReply(int code, const Client& user, const std::string& msg);
};
bool checkWatespaces(const std::string& str);
bool equalsIgnoreCase(const std::string& a, const std::string& b);
Command parse(std::string msg);
