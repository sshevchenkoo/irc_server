#pragma once

#include <string>
#include <ctime>

class Client
{
private:
	int _fd;
	std::string _buffIn;
	std::string _buffOutClient;

	std::string _nick;
	std::string _user;
	std::string _relName;
	std::string _host;

	bool _pass;
	bool _hasNick;
	bool _hasUser;
	bool _registred;
	bool _welcomed;

	std::time_t _last_active;
	bool _pongAwait;

public:
	Client();
	Client(int fd, std::string host);
	~Client();

	void appendInBuf (const char *data, std::size_t n);
	std::string& GetInBuff();
	std::string& GetOutBuff();

	void addToOutBuf(const std::string &s);
	bool WantsWrite() const;

	std::time_t lastActive() const;
	void UpdateActive();
	void tryRegistred();

	bool isAvatingPong() const;
	void SetPong(bool b);

	int getFd () const;

	const std::string& getNick() const;
	void setUser(const std::string &user, std::string &realname);
	const std::string& getUser() const;
	const std::string& getName() const;
	const std::string& getHost() const;

	void Pass();
	void applyNick(const std::string& nick);
	bool isRegistred() const;
	bool isWelcomed() const;
	void setWelcomed();

	std::string getMask () const;
};
