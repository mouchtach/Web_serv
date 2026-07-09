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
    std::vector<int > _cgiFds; // Store the fds of CGI processes

public:
    Webserv(){};

    bool is_server(int fd) const ;
    bool is_cgi(int fd) const  {
        for (size_t i = 0; i < _cgiFds.size(); ++i) {
            if (_cgiFds[i] == fd) {
                return true;
            }
        }
        return false;
    };
    void setupServers(const std::string &configFile);
    void Start();
    void newConnection(int serverFd);
    void readFromClient(int clientFd);

    Server* getServerByFd(int fd);
    Client* getClientByFd(int fd);
    pollfd* getPollfdByFd(int fd);
    Client* getClientCGI(int cgiFd);
    
    std::vector<pollfd>&getPollfds() { return _pollfds; }
    void readyToSend(int clientFd);
    void readFromCGI(int cgiFd);
    void removeClient(int clientFd);



    // void PrintServers() const;
};
