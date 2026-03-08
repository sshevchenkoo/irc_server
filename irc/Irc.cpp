#include "Irc.hpp"
#include <signal.h>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <cstdio>
#include "../Utils/utils.hpp"
#include "Channel.hpp"
#include "../Client/Client.hpp"
#include "../Server/Server.hpp"

std::map<std::string, IRC::handler> IRC::handlers;
std::map<int, std::string> IRC::numAnswers;

void IRC::initNumAnswers()
{
	numAnswers[ERR_NOSUCHNICK] = "No such nick/channel";
	numAnswers[ERR_NOORIGIN] = "No origin specified";
	numAnswers[ERR_NORECIPIENT] = "No recipient given";
	numAnswers[ERR_NOTEXTTOSEND] = "No text to send";

	numAnswers[RPL_ENDOFNAME] = "End of /NAMES list";
	numAnswers[RPL_NOTOPIC] = "No topic is set";

	numAnswers[ERR_UNKNOWNCOMMAND] = "Unknown command";

	numAnswers[ERR_NICKNAMEINUSE] = "Nickname is already in use";
	numAnswers[ERR_NONICKNAMEGIVEN] = "No nickname given";
	numAnswers[ERR_ERRONEUSNICKNAME] = "Erroneus nickname";

	numAnswers[ERR_USERNOTINCHANNEL] = "They aren't on that channel";
	numAnswers[ERR_NOTONCHANNEL] = "You're not on that channel";
	numAnswers[ERR_USERONCHANNEL] = "is already in channel";
	numAnswers[ERR_NOTREGISTERED] = "You have not registered";

	numAnswers[ERR_NEEDMOREPARAMS] = "Not enough parameters";
	numAnswers[ERR_ALREADYREGISTRED] = "You may not reregister";
	numAnswers[ERR_PASSWDMISMATCH] = "Password incorrect";
	numAnswers[ERR_KEYSET] = "Channel key already set";

	numAnswers[ERR_UNKNOWNMODE] = "is unknown mode char to me";
	numAnswers[ERR_BADCHANNELKEY] = "Cannot join channel (+k)";
	numAnswers[ERR_CHANNELISFULL] = "Cannot join channel (+l)";
	numAnswers[ERR_INVITEONLYCHAN] = "Cannot join channel (+i)";
	numAnswers[ERR_CHANOPRIVNEEDED] = "You're not channel operator";
}

void IRC::initHandlers()
{
	handlers["PASS"] = &handlePASS;
	handlers["NICK"] = &handleNICK;
	handlers["USER"] = &handleUSER;
	handlers["PING"] = &handlePING;
	handlers["PRIVMSG"] = &handlePRIVMSG;
	handlers["JOIN"] = &handleJOIN;
	handlers["PART"] = &handlePART;
	handlers["PONG"] = &handlePONG;
	handlers["MODE"] = &handleMODE;
	handlers["INVITE"] = &handleINVITE;
	handlers["KICK"] = &handleKICK;
	handlers["TOPIC"] = &handleTOPIC;
	handlers["QUIT"] = &handleQUIT;
}

bool IRC::extractOneMessage(std::string &buff, std::string &msg)
{
	std::size_t pos = buff.find("\n");
	if (pos == std::string::npos)
		return false;
	size_t end = pos;
	if (end > 0 && buff[end - 1] == '\r')
		end--;
	if (end > MAX_MESS_LEN)
		end = MAX_MESS_LEN;
	msg.assign(buff, 0, end);
	buff.erase(0, pos + 1);
	return true;
}

static inline void strToUpper(std::string &s)
{
	for (std::size_t i = 0; i < s.size(); i++)
		s[i] = toupper(s[i]);
}

std::string IRC::makeStringFromServ(const std::string &message)
{
	return (std::string(":") + SERVERNAME + " " + message + "\r\n");
}

std::string IRC::makeNumStringName(int n, const std::string &name, const std::string &prefix, std::string cmd, const std::string &trailing)
{
	char codeBuf[4];
	std::snprintf(codeBuf, sizeof(codeBuf), "%03d", n);

	std::string text = IRC::numAnswers.count(n) ? IRC::numAnswers[n] : trailing;

	std::string reply = std::string(":") + prefix + " " + std::string(codeBuf) + " " + name;

	if (!cmd.empty())
		reply += " " + cmd;

	if (!text.empty())
		reply += " :" + text;
	reply += "\r\n";

	LOG_DEBUG << "Sending Resposne: " << reply << std::endl;

	return reply;
}

std::string IRC::makeNumStringChannel(int n, Channel &channel, const std::string &prefix, std::string cmd, const std::string &trailing)
{
	return makeNumStringName(n, channel.getDisplayName(), prefix, cmd, trailing);
}

std::string IRC::makeNumString(int n, Client &client, const std::string &prefix, std::string cmd, const std::string &trailing)
{
	return makeNumStringName(n, client.getNick(), prefix, cmd, trailing);
}

void IRC::handleMessage(Server &s, Client &client, const std::string &msg)
{
	client.UpdateActive();

	command tempCmd = parseLine(msg);

	if (tempCmd.cmd.empty())
	{
		s.sendToClient(client, IRC::makeNumString(ERR_UNKNOWNCOMMAND, client, ""));
		return;
	}

	tempCmd.display();
	if (tempCmd.cmd == "PRIVMSG" || tempCmd.cmd == "JOIN" || tempCmd.cmd == "MODE" || tempCmd.cmd == "PART" || tempCmd.cmd == "INVITE" || tempCmd.cmd == "KICK" || tempCmd.cmd == "TOPIC")
	{
		if (!client.isRegistred())
		{
			s.sendToClient(client, IRC::makeNumString(ERR_NOTREGISTERED, client));
			return;
		}
	}
}

static void trimRight(std::string &s)
{
	while (!s.empty() && (s[s.size() - 1] == ' ' || s[s.size() - 1] == '\t'))
		s.erase(s.size() - 1);
}

IRC::command IRC::parseLine(std::string s)
{
	command res;
	trimRight(s);
	if (s.empty())
		return res;

	const char *p = s.c_str();
	const char *end = p + s.size();

	if (*p == ':')
	{
		++p;
		const char *start = p;
		while (p < end && *p != ' ')
			++p;
		res.prefix = std::string(start, p - start);
		while (p < end && *p == ' ')
			++p;
	}
	if (p >= end)
		return res;
	const char *cmdStart = p;
	while (p < end && *p != ' ')
		++p;
	res.cmd = std::string(cmdStart, p - cmdStart);

	strToUpper(res.cmd);

	while (p < end)
	{
		while (p < end && *p == ' ')
			++p;
		if (p >= end)
			break;

		if (*p == ':')
		{
			res.had_trailing = true;
			++p;
			res.trailing = std::string(p, end - p);
			break;
		}

		const char *argStart = p;
		while (p < end && *p != ' ')
			++p;
		res.params.push_back(std::string(argStart, p - argStart));
	}

	return res;
}
