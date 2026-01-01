#pragma once

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <csignal>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <sys/poll.h>
#include <cstddef>
#include <iostream>
#include "../utils/utils.hpp"
//#include "../channel/Channel.hpp"
//#include "../client/Client.hpp"

class Client;

#define CONNECTION_QUEUE_SIZE 100
#define POLL_TIMEOUT 200
#define READ_BUF_SIZE 1024

class Server {
private:
    int                     _listen_fd;
    int                     _port;
    std::string             _pass;
    sockaddr_in             _serv_addr;
    std::vector<pollfd>     _pfds;
    std::map<int, Client>   _clients;

    std::map<std::string, Channel> _channels;

    static pollfd makePfd(int fd);
    void    setEvents(int fd, short ev);
    Client& getClient(int fd);
    bool    handleRead(int fd);
    bool    handleWrite(int fd);
    void    acceptNewClients(std::vector<pollfd>& toAdd);

public:
    Server(int port, const std::string& p);

    void    sendToClient(Client& client, const std::string& line);
    void    init();
    void    run();
    void    tick(std::vector<int>& toDrop);

    int     setNick(Client& client, std::string& nick);
    void    tryRegister(Client&);
    Client* getClientByNick(const std::string& nick);

    Channel*    getChannelByName(const std::string &name);
    void        addChannel(Channel c);
    void        removeChannel(const std::string &name);

    const   std::string&  getPassword();
    void    removeClient(int fd);
    void    broadcastToCommonChannels(Client& client, const std::string& line );
};