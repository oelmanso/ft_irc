#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <sstream>
#include <vector>
#include <csignal>
#include <ctime>
#include <cerrno>

extern volatile sig_atomic_t g_running;

class Bot
{
	private:
		int					sock_fd;
		struct sockaddr_in	server_addr;

		std::string	host;
		int port;
		std::string	password;
		std::string	channel;
		std::string	nickname;
		std::string	username;
		std::string	realname;
		bool authenticated;
		bool registered;
		bool in_channel;
		std::string	buffer;		
		std::string	sendBuffer;	

		void	createSocket();
		void	connectToServer();
		void	setNonBlocking(int fd);
		void	queueRaw(const std::string &msg);
		void	sendPendingData();
		bool	hasPendingData() const;

		bool	receiveData();
		void	authenticate();
		void	joinChannel();
		void	handleMessage(const std::string& line);
		void	parseAndRespond(const std::string& sender, const std::string& message);
		void	sendMessage(const std::string& target, const std::string& text);

		// Bot commands
		void	cmdHelp(const std::string& sender);
		void	cmdPing(const std::string& sender);
		void	cmdTime(const std::string& sender);
		void	cmdEcho(const std::string& sender, const std::string& args);

	public:
		Bot();
		Bot(const std::string& host, int port, const std::string& password,
			 const std::string& channel, const std::string& nickname);
		~Bot();
		void run();
};