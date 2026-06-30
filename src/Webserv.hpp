#pragma once 

#include "../server/server.hpp"
#include "../server/client.hpp"
#include <vector>
#include <poll.h>

class Webserv
{
private:
    std::map<int, Client> _clientMap;
    std::vector<Server> _servers;
    std::vector<pollfd> _pollfds;
    std::vector<Client> _clients;

public:
    Webserv(){};

    bool is_server(int fd) const ;
    void setupServers(const std::string &configFile);
    void Start();
    void newConnection(int serverFd);
    void readFromClient(int clientFd);

    Server* getServerByFd(int fd);
    Client* getClientByFd(int fd);
    pollfd* getPollfdByFd(int fd);
    void readyToSend(int clientFd);
    void removeClient(int clientFd);


    // void PrintServers() const;
};
