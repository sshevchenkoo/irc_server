#pragma once

#include "Irc.hpp"
#include "../server/Server.hpp"
#include "../utils/utils.hpp"
//#include "../client/Client.hpp"

class Client;

enum ChannelModes
{
    MODE_INVITE_ONLY,
    MODE_TOPIC_OPERATOR_ONLY,
    MODE_USER_LIMIT,
    MODE_KEY_PROTECTED,
};

class Channel
{
    private:
        std::string                 _name;
        std::string                 _key;
        std::string                 _topic;
        std::set<const Client *>    _clients;
        std::set<const Client *>    _operators;
        std::set<const Client *>    _invited;

        size_t          _limit;
        unsigned char   _mode;
    public:
        Channel();
        Channel(std::string &name, Client *op);
        Channel(std::string &name, Client *op, std::string &key);
        ~Channel();

        Channel(const Channel &);
        Channel &operator=(const Channel &);

        void    addClient(const Client *client);
        void    removeClient(const Client *client);
        void    addOperator(const Client *client);
        void    removeOperator(const Client *client);
        void    addInvited(const Client *client);
        void    removeInvited(const Client *client);

        const std::string   &getKey(void) const;
        void                setKey(const std::string &key);
        const std::string   &getName(void) const;
        void                setName(const std::string &name);

        std::string             getDisplayName(void) const;
        std::queue<std::string> getUsersOnChannel(void) const;

        const std::set<const Client *>  &getClients(void) const;
        const std::set<const Client *>  &getOperators(void) const;

        bool    isOperator(const Client *op) const;
        bool    isInvited(const Client *op) const;

        void    setMode(int mode);
        void    unsetMode(int mode);
        int     getMode(void) const;
        bool    hasMode(int mode) const;

        std::string getModeString(void) const;

        void    setLimit(size_t limit);
        size_t  getLimit(void) const;

        bool    isFull(void) const;
        bool    isKey(const std::string &key) const;

        bool    hasClient(const Client *client) const;

        bool                hasTopic(void) const;
        const std::string   &getTopic(void) const;
        void                setTopic(std::string topic);

        void    handleModeSet(Server &S, Client &client, std::string modes, IRC::command &cmd);
        void    handleModeUnset(Server &S, Client &client, std::string modes, IRC::command &cmd);

        void    broadcast(Server &s, const std::string &msg) const;
};