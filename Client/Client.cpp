#include "Client.hpp"
#include "../irc/Irc.hpp"

Client::Client() {};

Client::Client(int fd, std::string host) : _fd(fd), _nick("*"), _user("*"), _host(host), _pass(false),
										   _hasNick(false), _hasUser(false), _registred(false), _last_active(std::time(NULL)), _pongAwait(false) {};

Client::~Client() {};

void Client::tryRegistred()
{
	if (_pass && _hasNick && _hasUser && !_registred)
		_registred = true;
}

void Client::appendInBuf(const char *data, std::size_t n)
{
	_buffIn.append(data, n);
}

std::string &Client::GetInBuff()
{
	return _buffIn;
}

std::string &Client::GetOutBuff()
{
	return _buffOutClient;
}

void Client::addToOutBuf(const std::string &s)
{
	std::string toAdd = s;
	if (s.size() > MAX_MESS_LEN + 2)
		toAdd = s.substr(0, MAX_MESS_LEN) + "\r\n";
	_buffOutClient += toAdd;
}

bool Client::WantsWrite() const
{
	return !_buffOutClient.empty();
}

std::time_t Client::lastActive() const
{
	return _last_active;
}

void Client::UpdateActive()
{
	_last_active = std::time(NULL);
}

bool Client::isAvatingPong() const
{
	return _pongAwait;
}

void Client::SetPong(bool b)
{
	_pongAwait = b;
}

int Client::getFd() const
{
	return _fd;
}

void Client::applyNick(const std::string &nick)
{
	_nick = nick;
	_hasNick = true;
	tryRegistred();
}

const std::string &Client::getNick() const
{
	return _nick;
}

void Client::setUser(const std::string &user, std::string &realname)
{
	_user = user;
	_relName = realname;
	_hasUser = true;
	tryRegistred();
}

const std::string &Client::getUser() const
{
	return _user;
}

const std::string &Client::getName() const
{
	return _relName;
}

const std::string &Client::getHost() const
{
	return _host;
}

void Client::Pass()
{
	_pass = true;
	tryRegistred();
}

bool Client::isRegistred() const
{
	return _registred;
}

std::string Client::getMask() const
{
	return _nick + "!" + _user + "@" + _host;
}

bool Client::isWelcomed() const
{
	return _welcomed;
}

void Client::setWelcomed()
{
	_welcomed = true;
}
