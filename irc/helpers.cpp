#include "Irc.hpp"
#include "../Utils/utils.hpp"
#include "../Server/Server.hpp"
#include "../Client/Client.hpp"
#include "Channel.hpp"

bool checkNoCommand(Server &S, Client &client, IRC::command &cmd, int errNum)
{
	if (cmd.params.empty() && cmd.trailing.empty())
	{
		S.sendToClient(client, IRC::makeNumString(errNum, client));
		return true;
	}
	return false;
}

bool isValidNick(const std::string &nick)
{
	if (nick.empty() || nick.length() > 9)
		return false;
	if (isdigit(nick[0]) || nick[0] == '-')
		return false;

	for (size_t i = 0; i < nick.length(); i++)
	{
		char c = nick[i];
		if (!isalnum(c) &&
			c != '[' && c != ']' && c != '\\' && c != '`' &&
			c != '_' && c != '^' && c != '{' && c != '|' && c != '}')
		{
			return false;
		}
	}
	return true;
}

// bool checkNoCommand(Server& S, Client& client,  IRC::command& cmd, int errNum){
//     if (cmd.params.empty()) {S.sendToClient(client, IRC::makeNumString(411, client)); return true;}
//     if (cmd.trailing.empty()) {S.sendToClient(client, IRC::makeNumString(412, client)); return true;}
//     return false;
// }

void IRC::handlePASS(Server &S, Client &client, IRC::command &cmd)
{
	if (client.isRegistred())
		S.sendToClient(client, IRC::makeNumString(ERR_ALREADYREGISTRED, client));
	else if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;
	else if (!cmd.params.empty() && cmd.params[0] != S.getPassword())
		S.sendToClient(client, IRC::makeNumString(ERR_PASSWDMISMATCH, client));
	else
	{
		client.Pass();
		S.tryRegister(client);
	}
}

void IRC::handleNICK(Server &S, Client &client, IRC::command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NONICKNAMEGIVEN))
		return;

	std::string &nick = cmd.params[0];
	if (!isValidNick(nick))
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_ERRONEUSNICKNAME, nick));
		return;
	}
	bool wasReg = client.isRegistred();
	std::string oldMask = client.getMask();
	if (S.setNick(client, nick) == ERR_NICKNAMEINUSE)
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NICKNAMEINUSE, client, nick));
		return;
	}

	S.tryRegister(client);
	if (wasReg)
	{
		std::string msg = ":" + oldMask + " NICK :" + nick + "\r\n";
		S.sendToClient(client, msg);
		S.broadcastToCommonChannels(client, msg);
	}
}

void IRC::handleUSER(Server &S, Client &client, IRC::command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;
	if (cmd.params.size() < 3)
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client, "SuperServ" "USER"));
		return;
	}
	std::string realname = cmd.trailing;
	if(realname.empty() && cmd.params.size() >= 4)
		realname = cmd.params[3];
	if(realname.empty())
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client, "SuperServ" "USER"));
		return;
	}
	client.setUser(cmd.params[0], realname);
	S.tryRegister(client);
}

void IRC::handlePING(Server &S, Client &client, IRC::command &cmd)
{
	client.UpdateActive();
	if (cmd.params.empty() && cmd.trailing.empty())
		S.sendToClient(client, IRC::makeNumString(ERR_NOORIGIN, client));
	else
	{
		std::string tok = cmd.params.empty() ? cmd.trailing : cmd.params[0];
		LOG_DEBUG << "Replying to ping" << std::endl;
		S.sendToClient(client, IRC::makeStringFromServ(std::string("PONG ") + SERVERNAME + " :" + tok));
	}
}

void IRC::handlePONG(Server &S, Client &client, IRC::command &cmd)
{
	(void)S;
	(void)cmd;
	client.UpdateActive();
	client.SetPong(false);
}

void IRC::handlePRIVMSG(Server &S, Client &client, IRC::command &cmd)
{
	if (cmd.params.empty())
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NORECIPIENT, client));
		return;
	}
	if (cmd.trailing.empty())
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NOTEXTTOSEND, client));
		return;
	}

	std::string &target = cmd.params[0];

	if (target[0] == '#' || target[0] == '&')
	{
		std::string target_name = target.substr(1);
		Channel *c = S.getChannelByName(target_name);
		if (!c)
		{
			S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, target));
			return;
		}
		else if (!c->hasClient(&client))
		{
			S.sendToClient(client, IRC::makeNumStringChannel(ERR_NOTONCHANNEL, *c));
			return;
		}
		LOG_DEBUG << "Sending message to channel " << c->getName() << std::endl;
		LOG_DEBUG << "Channel has " << c->getClients().size() << " members" << std::endl;
		std::string msg = ":" + client.getMask() + " PRIVMSG " + "#" + c->getName() + " :" + cmd.trailing + "\r\n";
		std::set<const Client *>::iterator it = c->getClients().begin();

		while (it != c->getClients().end())
		{
			if (*it != &client)
			{
				S.sendToClient(*const_cast<Client *>(*it), msg);
			}
			it++;
		}
	}
	else
	{
		Client *reciept = S.getClientByNick(cmd.params[0]);
		if (!reciept)
		{
			S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHNICK, client.getNick(), cmd.params[0]));
			return;
		}
		S.sendToClient(*reciept,
					   ":" + client.getMask() + " PRIVMSG " + reciept->getNick() + " :" + cmd.trailing + "\r\n");
	}
}

void IRC :: handleCAP(Server &Server, Client &Client, IRC::command &cmd)
{
	if(cmd.params.empty())
		return;
	std::string subcmd = cmd.params[0];
	if(subcmd == "LS")
	{
		Server.sendToClient(Client, ":" + std::string("SuperServ") + " CAP " + Client.getNick()
	+ " LS:\r\n" );
	}
}

void IRC::handleQUIT(Server &S, Client &client, IRC::command &cmd)
{

	std::string farewell = cmd.trailing.empty() ? "abducted by aliens" : cmd.trailing;
	S.broadcastToCommonChannels(client, ":" + client.getMask() + " QUIT :" + farewell + "\r\n");

	S.removeClient(client.getFd());
}
void IRC::handleJOIN(Server &S, Client &client, IRC::command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;

	std::string target = cmd.params[0];
	LOG_DEBUG << "received join command" << std::endl;

	if (target[0] != '#' && target[0] != '&')
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHCHANNEL, client, SERVERNAME, target));
		return;
	}

	std::string channel_name = target.substr(1);
	Channel *c = S.getChannelByName(channel_name);
	std::string key = (cmd.params.size() > 1) ? cmd.params[1] : "";

	if (!c)
	{
		LOG_DEBUG << "Creating channel " << channel_name << std::endl;
		Channel new_channel(channel_name, &client, key);
		if (!key.empty())
		{
			new_channel.setMode(MODE_KEY_PROTECTED);
		}
		S.addChannel(new_channel);
		c = S.getChannelByName(channel_name);
	}
	else
	{
		if (c->hasMode(MODE_INVITE_ONLY) && !c->isInvited(&client))
		{
			S.sendToClient(client, IRC::makeNumStringName(ERR_INVITEONLYCHAN, client.getNick() + " " + c->getDisplayName()));
			return;
		}
		if (c->isFull() && !c->isInvited(&client))
		{
			S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANNELISFULL, *c));
			return;
		}
		if (c->hasMode(MODE_KEY_PROTECTED) && !c->isKey(key) && !c->isInvited(&client))
		{
			S.sendToClient(client, IRC::makeNumStringChannel(ERR_BADCHANNELKEY, *c));
			return;
		}
	}
	c->addClient(&client);

	std::string joinMsg = ":" + client.getMask() + " JOIN :" + c->getDisplayName() + "\r\n";
	for (std::set<const Client *>::iterator it = c->getClients().begin();
		 it != c->getClients().end(); ++it)
	{
		S.sendToClient(*const_cast<Client *>(*it), joinMsg);
	}

	if (c->hasTopic())
	{
		S.sendToClient(client, IRC::makeNumString(RPL_TOPIC, client, SERVERNAME,
												  c->getDisplayName(), c->getTopic()));
	}

	std::queue<std::string> names = c->getUsersOnChannel();
	std::stringstream origMsg;
	origMsg << ":" << SERVERNAME << " " << RPL_NAMREPLY << " "
			<< client.getNick() << " " << c->getDisplayName() << " :";
	std::string sendingMsg = origMsg.str();

	while (!names.empty())
	{
		std::string name = names.front();
		names.pop();

		if (sendingMsg.size() + name.size() + 1 > MAX_MESS_LEN)
		{
			S.sendToClient(client, sendingMsg + "\r\n");
			sendingMsg = origMsg.str();
		}
		sendingMsg += name + " ";
	}

	if (sendingMsg != origMsg.str())
	{
		S.sendToClient(client, sendingMsg + "\r\n");
	}

	S.sendToClient(client, IRC::makeNumStringName(RPL_ENDOFNAME,
												  client.getNick() + " " + c->getDisplayName()));
}

void handleChannelMODE(Server &S, Client &client, IRC::command &cmd)
{
	if (!client.isRegistred())
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NOTREGISTERED, client));
		return;
	}
	std::string target = cmd.params[0].substr(1);
	Channel *chan = S.getChannelByName(target);

	if (chan)
	{
		if (cmd.params.size() > 1)
		{
			if (chan->isOperator(&client))
			{
				std::string modes = cmd.params[1];
				if (modes[0] == '+')
				{
					chan->handleModeSet(S, client, modes.substr(1), cmd);
				}
				else if (modes[0] == '-')
				{
					chan->handleModeUnset(S, client, modes.substr(1), cmd);
				}
				else
				{
					S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE,
																  cmd.params[1].empty() ? "" : std::string(1, cmd.params[1][0])));
				}
			}
			else
			{
				S.sendToClient(client, IRC::makeNumString(ERR_CHANOPRIVNEEDED, client));
			}
		}
		else
		{

			S.sendToClient(client, IRC::makeNumStringName(RPL_CHANNELMODEIS, chan->getDisplayName() + " " + chan->getModeString()));
		}
	}
	else
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHCHANNEL, client));
	}
}

void handleUserMODE(Server &S, Client &client, IRC::command &cmd)
{
	(void)S;
	(void)client;
	(void)cmd;
}

void IRC::handleMODE(Server &S, Client &client, IRC::command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;
	std::string &target = cmd.params[0];
	if (!target.empty() && (target[0] == '#' || target[0] == '&'))
	{
		handleChannelMODE(S, client, cmd);
	}
	else
	{
		handleUserMODE(S, client, cmd);
	}
}

void IRC::handlePART(Server &S, Client &client, IRC::command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;
	std::vector<std::string> targets = split(cmd.params[0], ',');

	for (size_t i = 0; i < targets.size(); i++)
	{
		std::string target = targets[i];
		if (target[0] != '#' && target[0] != '&')
		{
			S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHCHANNEL, client, SERVERNAME, target));
			continue;
		}
		std::string channel_name = target.substr(1);
		Channel *c = S.getChannelByName(channel_name);
		if (!c)
		{
			S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHCHANNEL, client, SERVERNAME, target));
			continue;
		}
		if (!c->hasClient(&client))
		{
			S.sendToClient(client, IRC::makeNumStringChannel(ERR_NOTONCHANNEL, *c));
			continue;
		}
		std::string msg = ":" + client.getMask() + " PART " + c->getDisplayName();
		if (!cmd.trailing.empty())
		{
			msg += " :" + cmd.trailing;
		}
		msg += "\r\n";
		for (std::set<const Client *>::iterator it = c->getClients().begin(); it != c->getClients().end(); it++)
		{
			S.sendToClient(*const_cast<Client *>(*it), msg);
		}
		c->removeClient(&client);
		if (c->getClients().empty())
		{
			S.removeChannel(c->getName());
		}
	}
}

void IRC::handleINVITE(Server &S, Client &client, command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;

	if (cmd.params.size() < 2)
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
		return;
	}

	if (cmd.params[1][0] != '#' && cmd.params[1][0] != '&')
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, cmd.params[1]));
		return;
	}

	std::string targetChannelName = cmd.params[1].substr(1);

	Client *targetClient = S.getClientByNick(cmd.params[0]);
	Channel *targetChannel = S.getChannelByName(targetChannelName);
	if (!targetChannel)
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, targetChannelName));
	}
	else if (!targetChannel->isOperator(&client))
	{
		S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANOPRIVNEEDED, *targetChannel));
	}
	else if (!targetClient)
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHNICK, client.getNick(), cmd.params[0]));
	}
	else if (targetChannel->hasClient(targetClient))
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_USERONCHANNEL, client.getNick() + " " + targetClient->getNick() + " " + targetChannel->getDisplayName()));
	}
	else
	{
		S.sendToClient(*targetClient, ":" + client.getMask() + " INVITE " + targetClient->getNick() +
										  " :" + targetChannel->getDisplayName() + "\r\n");
		S.sendToClient(client, IRC::makeNumStringName(RPL_INVITING, targetChannel->getDisplayName() + " " + targetClient->getNick()));
		targetChannel->addInvited(targetClient);
	}
}

void IRC::handleKICK(Server &S, Client &client, command &cmd)
{
	(void)S;
	(void)client;
	(void)cmd;
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;

	if (cmd.params.size() < 2)
	{
		S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
		return;
	}

	if (cmd.params[0][0] != '#' && cmd.params[0][0] != '&')
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, cmd.params[0]));
		return;
	}

	std::string targetChannelName = cmd.params[0].substr(1);

	Client *targetClient = S.getClientByNick(cmd.params[1]);
	Channel *targetChannel = S.getChannelByName(targetChannelName);

	if (!targetChannel)
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, cmd.params[0]));
	}
	else if (!targetChannel->isOperator(&client))
	{
		S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANOPRIVNEEDED, *targetChannel));
	}
	else if (!targetClient)
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHNICK, client.getNick(), cmd.params[0]));
	}
	else if (!targetChannel->hasClient(targetClient))
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_USERNOTINCHANNEL, client.getNick() + " " + targetClient->getNick() + " " + targetChannel->getDisplayName()));
	}
	else
	{
		std::string trail = cmd.trailing.empty() ? client.getNick() : cmd.trailing;
		std::string kickMsg = ":" + client.getMask() + " KICK " + targetChannel->getDisplayName() + " " + targetClient->getNick() + " :" + trail + "\r\n";

		LOG_DEBUG << "Kick message " << kickMsg << std::endl;

		targetChannel->broadcast(S, kickMsg);
		S.sendToClient(*targetClient, kickMsg);
		targetChannel->removeInvited(targetClient);
		targetChannel->removeClient(targetClient);
	}
}

void IRC::handleTOPIC(Server &S, Client &client, command &cmd)
{
	if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
		return;
	if (cmd.params[0][0] != '#' && cmd.params[0][0] != '&')
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, cmd.params[0]));
		return;
	}

	Channel *targetChannel = S.getChannelByName(cmd.params[0].substr(1));

	if (!targetChannel)
	{
		S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, cmd.params[0]));
	}
	else if (!targetChannel->hasClient(&client))
	{
		S.sendToClient(client, IRC::makeNumStringChannel(ERR_NOTONCHANNEL, *targetChannel));
	}
	else if (cmd.trailing.empty())
	{
		if (cmd.had_trailing)
		{
			targetChannel->setTopic("");
			std::string topicMsg = ":" + client.getMask() + " TOPIC " + targetChannel->getDisplayName() + " :" + "\r\n";
			targetChannel->broadcast(S, topicMsg);
		}
		else if (targetChannel->getTopic().empty())
		{
			S.sendToClient(client, IRC::makeNumStringChannel(RPL_NOTOPIC, *targetChannel));
		}
		else
		{
			S.sendToClient(client, IRC::makeNumStringChannel(RPL_TOPIC, *targetChannel, SERVERNAME, "", targetChannel->getTopic()));
		}
	}
	else if (targetChannel->hasMode(MODE_TOPIC_OPERATOR_ONLY) && !targetChannel->isOperator(&client))
	{
		S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANOPRIVNEEDED, *targetChannel));
	}
	else
	{
		targetChannel->setTopic(cmd.trailing);
		std::string topicMsg = ":" + client.getMask() + " TOPIC " + targetChannel->getDisplayName() + " :" + targetChannel->getTopic() + "\r\n";

		targetChannel->broadcast(S, topicMsg);
	}
}
