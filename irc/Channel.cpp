#include "Channel.hpp"

Channel::Channel() {}

Channel::Channel(std::string &name, Client *op) :
    _name(name), _key(""), _clients(std::set<const Client *>()),
    _operators(std::set<const Client *>())
{
    _clients.insert(op);
    _operators.insert(op);
}

Channel::Channel(std::string &name, Client *op, std::string &key) :
    _name(name), _key(key), _clients(std::set<const Client *>()),
    _operators(std::set<const Client *>())
{
    _clients.insert(op);
    _operators.insert(op);
}

Channel::~Channel() {}

Channel::Channel(const Channel& other)
{
    if (this != &other)
        *this = other;
}

Channel &Channel::operator=(const Channel& other)
{
    if (this != &other)
    {
        this->_clients = other._clients;
        this->_name = other._name;
        this->_operators = other._operators;
    }
    return (*this);
}

void Channel::addClient(const Client *client)
{
    _clients.insert(client);
}

void Channel::removeClient(const Client *client)
{
    _clients.erase(client);
}

void Channel::addOperator(const Client *op)
{
    _operators.insert(op);
}

void Channel::removeOperator(const Client *op)
{
    _operators.erase(op);
}

void Channel::addInvited(const Client *client)
{
    _invited.insert(client);
}

void Channel::removeInvited(const Client *client)
{
    _invited.erase(client);
}

const std::set<const Client *> &Channel::getClients(void) const
{
    return _clients;
}

const std::set<const Client *> &Channel::getOperators(void) const
{
    return _operators;
}

const std::string &Channel::getName(void) const
{
    return _name;
}

std::string Channel::getDisplayName(void) const
{
    return std::string("#") + _name;
}

void Channel::setName(const std::string &name)
{
    _name = name;
}

const std::string &Channel::getKey(void) const
{
    return _key;
}

void Channel::setKey(const std::string &key)
{
    _key = key;
}

bool Channel::isOperator(const Client *op) const
{
    return _operators.find(op) != _operators.end();
}

bool Channel::isInvited(const Client *client) const
{
    return _invited.count(client) != 0;
}

int Channel::getMode(void) const
{
    return _mode;
}

void Channel::unsetMode(int mode)
{
    _mode &= ~(1 << mode);
}

void Channel::setMode(int mode)
{
    _mode |= (1 << mode);
}

bool Channel::hasMode(int mode) const
{
    return _mode & (1 << mode);
}

void Channel::setLimit(size_t limit)
{
    _limit = limit;
}

size_t Channel::getLimit(void) const
{
    return _limit;
}

bool Channel::isFull(void) const
{
    return hasMode(MODE_USER_LIMIT) && _clients.size() == _limit;
}

bool Channel::isKey(const std::string &key) const
{
    return _key == key;
}

bool Channel::hasClient(const Client *client) const
{
    return _clients.count(client) > 0;
}

bool Channel::hasTopic(void) const
{
    return !_topic.empty();
}

const std::string& Channel::getTopic(void) const
{
    return _topic;
}

void Channel::setTopic(std::string topic) {
    _topic = topic;
}

std::string Channel::getModeString(void) const
{
    std::string modes = "+";
    std::stringstream params;
    if (hasMode(MODE_INVITE_ONLY))
        modes.push_back('i');
    if (hasMode(MODE_TOPIC_OPERATOR_ONLY))
        modes.push_back('t');
    if (hasMode(MODE_KEY_PROTECTED))
        modes.push_back('k');
    if (hasMode(MODE_USER_LIMIT))
    {
        modes.push_back('l');
        params << " " << _limit;
    }
    if (!params.str().empty())
        modes += params.str();
    return modes;
}

std::queue<std::string> Channel::getUsersOnChannel(void) const
{
    std::set<const Client *>::iterator it = _clients.begin();
    std::queue<std::string> users;

    while (it != _clients.end())
    {
        if (isOperator(*it))
            users.push("@" + (*it)->getNick());
        else
            users.push((*it)->getNick());
        it++;
    }
    return users;
}

void Channel::handleModeSet(Server &S, Client &client, std::string modes, IRC::command &cmd)
{
    std::queue<std::string> args;
    std::string successfulModes = "";
    std::vector<std::string> successfulParams;

    for (size_t i = 2; i < cmd.params.size(); i++)
    {
        args.push(cmd.params[i]);
    }

    LOG_DEBUG << "mode in handleModeSet " << modes << std::endl;

    for (size_t i = 0; i < modes.size(); i++)
    {
        char c = modes[i];
        LOG_DEBUG << "mode in loop " << c << std::endl;
        switch (c)
        {
            case 'i':
                this->setMode(MODE_INVITE_ONLY);
                successfulModes += c;
                break;
            case 't':
                this->setMode(MODE_TOPIC_OPERATOR_ONLY);
                successfulModes += c;
                break;
            case 'o':
                if (args.empty())
                    S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                else
                {
                    std::string param = args.front();
                    args.pop();
                    Client *targetClient = S.getClientByNick(param);
                    if (!targetClient)
                        S.sendToClient(client, IRC::makeNumStringName(ERR_USERNOTINCHANNEL, cmd.params[2] + this->getDisplayName()));
                    if (this->hasClient(targetClient))
                    {
                        if (!this->isOperator(targetClient))
                        {
                            this->addOperator(targetClient);
                            successfulModes += c;
                            successfulParams.push_back(param);
                        }
                    }
                }
                break;
            case 'l':
                if (args.empty())
                    S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                else
                {
                    std::string param = args.front();
                    args.pop();
                    this->setMode(MODE_USER_LIMIT);
                    std::stringstream s(param);
                    size_t limit = 0;
                    s >> limit;
                    if (s.fail())
                        S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s.str()));
                    else
                    {
                        this->setLimit(limit);
                        successfulModes += c;
                        successfulParams.push_back(param);
                    }
                }
                break;
            case 'k':
                if (args.empty())
                    S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                else
                {
                    std::string param = args.front();
                    args.pop();
                    LOG_DEBUG << "key param " << param << std::endl;
                    if (this->hasMode(MODE_KEY_PROTECTED))
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_NEEDMOREPARAMS, *this));
                    else
                    {
                        this->setMode(MODE_KEY_PROTECTED);
                        this->setKey(param);
                        successfulModes += c;
                        successfulParams.push_back(param);
                    }
                }
                break;
            default:
                break;
        }
    }
    if (!successfulModes.empty())
    {
        std::string modeChangeMsg = ":" + client.getMask() + " MODE " + getDisplayName() + " +" + modes;
        for (size_t i = 0; i < successfulParams.size(); i++)
        {
            modeChangeMsg += " " + successfulParams[i];
        }
        modeChangeMsg += "\r\n";
        broadcast(S, modeChangeMsg);
    }
}

void    Channel::broadcast(Server &s, const std::string &msg) const
{
    std::set<const Client *>::iterator it = getClients().begin();

    while (it != getClients().end())
    {
        LOG_DEBUG << "Sending to client: " << (*it)->getNick() << std::endl;
        s.sendToClient(*const_cast<Client *>(*it), msg);
        it++;
    }
}

void Channel::handleModeUnset(Server &S, Client &client, std::string modes, IRC::command &cmd)
{
    std::queue<std::string> args;
    for (size_t i = 2; i < cmd.params.size(); i++)
        args.push(cmd.params[i]);

    for (size_t i = 0; i < modes.size(); i++)
    {
        char c = modes[i];
        switch (c)
        {
            case 'i':
                this->unsetMode(MODE_INVITE_ONLY);
                break;
            case 't':
                this->unsetMode(MODE_TOPIC_OPERATOR_ONLY);
                break;
            case 'o':
                if (args.empty())
                    S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                else
                {
                    std::string param = args.front();
                    args.pop();
                    Client *targetClient = S.getClientByNick(param);
                    if (!targetClient) {
                        S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHNICK, client));
                        return ;
                    }
                    if (this->isOperator(targetClient)) {
                        this->removeOperator(targetClient);
                    } else {
                        S.sendToClient(client, IRC::makeNumStringName(ERR_USERNOTINCHANNEL, param + " " + this->getDisplayName()));
                        return ;
                    }
                }
                break;
            case 'l':
                this->unsetMode(MODE_USER_LIMIT);
                this->setLimit(0);
                break;
            case 'k':
                this->unsetMode(MODE_KEY_PROTECTED);
                this->setKey("");
                break;
            default:
                S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, std::string() + c));
                return ;
        }
    }
}