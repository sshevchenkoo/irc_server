#include "Irc.hpp"
#include <signal.h>
#include <string>
#include <vector>
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

	/* all errors will be here*/

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
