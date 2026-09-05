#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

class Client
{
  private:
	int fd;
	std::string ip_address;
	std::string nickname;
	std::string username;
	std::string realname;
	std::string buffer;
	std::string sendBuffer;
	bool is_connected;
	bool is_authenticated;
	bool is_registered;

	Client(const Client &other);
	Client &operator=(const Client &other);

  public:
	Client();
	Client(int fd, const std::string &ip);
	~Client();

	// getters
	int get_fd() const;
	std::string get_ip_address() const;
	std::string get_nickname() const;
	std::string get_username() const;
	std::string get_realname() const;
	std::string getPrefix() const;
    const std::string& getBuffer() const;
	void append_to_buffer(const std::string &data);
	void clear_buffer();
	std::string	&getSendBuffer();
    bool getAuthenticated() const;
    bool getRegistered() const;
    bool getConnected() const;

	// setters
	void set_ip_address(const std::string &ip);
	void set_nickname(const std::string &nick);
	void set_username(const std::string &user);
	void set_realname(const std::string &real);
    void setAuthenticated(bool auth);
    void setRegistered(bool regi);
    void setConnected(bool connect);
	void append_to_sendBuffer(const std::string &data);
	void clear_sendBuffer(size_t n);
	bool hasPendingData() const;
};