#include "server.hpp"
#include <cerrno>
#include "signal.hpp"

Server::Server()
{
	server_fd = -1;
	port = 0;
	password = "";
}

Server::Server(int port, const std::string &password) : server_fd(-1),
	serverAddress(), port(port), password(password)
{
	create_socket();
	setsockpt();
	bind_socket();
	listen_socket();
	pollfd server_poll;
	server_poll.fd = server_fd;
	server_poll.events = POLLIN;
	server_poll.revents = 0;
	poll_fds.push_back(server_poll);
}

Server::~Server()
{
	if (server_fd != -1)
		close(server_fd);
	for (std::map<int, Client*>::iterator it = clients.begin();
		 it != clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	for (size_t i = 0; i < channels.size(); i++)
		delete channels[i];
	clients.clear();
	channels.clear();
}

void Server::run()
{
	std::cout << "Server is running on port " << port << std::endl;
	while (g_running)
	{
		int activity = poll(&poll_fds[0], poll_fds.size(), -1);
		if (activity < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Poll error");
		}
		for (size_t i = 0; i < poll_fds.size(); )
		{
			short revents = poll_fds[i].revents;
			poll_fds[i].revents = 0;
			int fd = poll_fds[i].fd;
			if (revents == 0)
			{
				i++;
				continue;
			}
			if (fd == server_fd)
			{
				if (revents & POLLIN)
					accept_client();
				if (revents & (POLLERR | POLLHUP | POLLNVAL))
					std::cerr << "poll() error on listening socket" << std::endl;
				i++;
				continue;
			}
			bool disconnected = false;

			if (revents & POLLIN)
			{
				receive_data(fd);
				if (i >= poll_fds.size() || poll_fds[i].fd != fd)
					disconnected = true;
			}
			if (!disconnected && (revents & POLLOUT))
			{
				sendPendingData(fd);
				if (i >= poll_fds.size() || poll_fds[i].fd != fd)
					disconnected = true;
			}
			if (!disconnected && !(revents & (POLLIN | POLLOUT)) &&
				(revents & (POLLHUP | POLLERR | POLLNVAL)))
			{
				disconnect_client(fd);
				disconnected = true;
			}
			if (!disconnected)
				i++;
		}
	}
	std::cout << std::endl;
	std::cout << "Server shutting down..." << std::endl;
}

void Server::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags == -1)
    	throw std::runtime_error("fcntl F_GETFL failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    	throw std::runtime_error("fcntl F_SETFL failed");
}

void Server::create_socket()
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (serverSocket == -1)
		throw std::runtime_error("Failed to create socket");
	server_fd = serverSocket;
	setNonBlocking(server_fd);
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
}

void Server::setsockpt()
{
	int opt = 1;

	if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("Failed to set socket options");
}

void Server::bind_socket()
{
	int socketBind;

	socketBind = bind(server_fd, (struct sockaddr *)&serverAddress,
			sizeof(serverAddress));
	if (socketBind == -1)
		throw std::runtime_error("Failed to bind socket");
}

void Server::listen_socket()
{
	int socketListen;

	socketListen = listen(server_fd, 10);
	if (socketListen == -1)
		throw std::runtime_error("Failed to listen on socket");
}

void Server::accept_client()
{
	sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientSocket = accept(server_fd, (sockaddr *)&clientAddr, &clientLen);

	if (clientSocket == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			perror("accept");
		return;
	}
	setNonBlocking(clientSocket);
	std::string ip = inet_ntoa(clientAddr.sin_addr);
	Client *client = new Client(clientSocket, ip);
	clients[clientSocket] = client;
	pollfd client_poll;
	client_poll.fd = clientSocket;
	client_poll.events = POLLIN;
	client_poll.revents = 0;
	poll_fds.push_back(client_poll);
	std::cout << "New client connected: "
		<< ip << " (fd = " << clientSocket << ")" << std::endl;
}

void Server::receive_data(int client_fd)
{
	char buffer[1024];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		disconnect_client(client_fd);
		return;
	}
	if (bytesRead == 0)
	{
		disconnect_client(client_fd);
		return;
	}
	buffer[bytesRead] = '\0';
	std::map<int, Client*>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;
	Client *client = it->second;
	client->append_to_buffer(buffer);
	std::string data = client->getBuffer();
	size_t pos;

	while ((pos = data.find("\n")) != std::string::npos)
	{ 
		std::string line = data.substr(0, pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
        	line.erase(line.size() - 1);
		std::cout << "Received from client "
			<< client_fd << ": " << line << std::endl;
		std::string response = handleCommand(*client, line);
		if (!response.empty())
			sendToClient(client_fd, response);
		data.erase(0, pos + 1);
		if (clients.find(client_fd) == clients.end())
			return;
	}
	client->clear_buffer();
	client->append_to_buffer(data);
}


void Server::sendToClient(int client_fd, const std::string &msg)
{
	std::map<int, Client*>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;

	Client *client = it->second;
	client->append_to_sendBuffer(msg);
	updatePollEvents(client_fd);
}

void Server::sendPendingData(int client_fd)
{
	std::map<int, Client*>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;
	Client *client = it->second;
	std::string &buf = client->getSendBuffer();

	if (buf.empty())
	{
		updatePollEvents(client_fd);
		return;
	}
	ssize_t sent = send(client_fd, buf.c_str(), buf.size(), 0);
	if (sent == 0)
    	return;
	if (sent < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{			
			updatePollEvents(client_fd);
			return;
		}
		disconnect_client(client_fd);
		return;
	}
	client->clear_sendBuffer((size_t)sent);
	updatePollEvents(client_fd);
}

void Server::updatePollEvents(int client_fd)
{
	std::map<int, Client*>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;

	bool pending = it->second->hasPendingData();

	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		if (poll_fds[i].fd == client_fd)
		{
			poll_fds[i].events = POLLIN | (pending ? POLLOUT : 0);
			break;
		}
	}
}

void Server::disconnect_client(int client_fd)
{
	std::map<int, Client*>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;

	removeClientFromChannels(client_fd);
	close(client_fd);
	delete it->second;
	for (std::vector<pollfd>::iterator pollIt = poll_fds.begin();
		 pollIt != poll_fds.end(); ++pollIt)
	{
		if (pollIt->fd == client_fd)
		{
			poll_fds.erase(pollIt);
			break;
		}
	}
	clients.erase(it);
}

void Server::removeClientFromChannels(int client_fd)
{
	for (std::vector<Channel *>::iterator channelIt = channels.begin();
		 channelIt != channels.end(); ++channelIt)
	{
		Channel *channel = *channelIt;
		int userIndex = channel->plase_user_in_channel(client_fd);
		if (userIndex != -1)
			channel->removeUser(userIndex);
		if (channel->is_operator(client_fd))
			channel->remove_operator(client_fd);
		if (channel->is_invitedUser(client_fd))
			channel->remove_invitedUser(client_fd);
	}
}
