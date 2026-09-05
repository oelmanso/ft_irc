#include "client.hpp"
#include <unistd.h>

Client::Client() : fd(-1), is_connected(false), 
is_authenticated(false), is_registered(false) {}

Client::Client(int fd, const std::string &ip) : fd(fd), 
	ip_address(ip), is_connected(true), 
	is_authenticated(false), is_registered(false) {}

Client::~Client() {}

int Client::get_fd() const
{
	return (fd);
}

std::string Client::get_ip_address() const
{
	return (ip_address);
}

std::string Client::get_nickname() const
{
	return (nickname);
}

std::string Client::get_username() const
{
	return (username);
}

std::string Client::get_realname() const
{
	return (realname);
}

std::string Client::getPrefix() const
{
	return (":" + nickname + "!" + username + "@" + ip_address);
}

std::string& Client::getSendBuffer()
{
	return (sendBuffer);
}

const std::string& Client::getBuffer() const
{
    return (buffer);
}

bool Client::getAuthenticated() const
{
    return (is_authenticated);
}

bool Client::getRegistered() const
{
    return (is_registered);
}

bool Client::getConnected() const
{
    return (is_connected);
}

void Client::set_ip_address(const std::string &ip)
{
	ip_address = ip;
}

void Client::set_nickname(const std::string &nick)
{
	nickname = nick;
}

void Client::set_username(const std::string &user)
{
	username = user;
}

void Client::set_realname(const std::string &real)
{
	realname = real;
}

void Client::setAuthenticated(bool auth)
{
    is_authenticated = auth;
}

void Client::setRegistered(bool regi)
{
    is_registered = regi;
}

void Client::setConnected(bool connect)
{
    is_connected = connect;
}

void Client::append_to_buffer(const std::string &data)
{
	buffer += data;
}

void Client::clear_buffer()
{
	buffer.clear();
}

void Client::append_to_sendBuffer(const std::string &data)
{
	sendBuffer += data;
}

void Client::clear_sendBuffer(size_t n)
{
	if (n >= sendBuffer.size())
		sendBuffer.clear();
	else
		sendBuffer.erase(0, n);
}

bool Client::hasPendingData() const
{
    return !sendBuffer.empty();
}