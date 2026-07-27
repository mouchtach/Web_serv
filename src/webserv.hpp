
#pragma once
#include "../parssing/config.hpp"
#include "server.hpp"
#include "client.hpp"

#include <map>
#include <vector>
#include <string>
#include <poll.h>
#include <unistd.h>

enum FD_type {
    FD_SERVER,
    FD_CLIENT,
    CGI
};

typedef struct FD_info {
    int fd;
    FD_type type;
    void *obj;
} FD_info;


class WebServ {
private:
    std::vector<Config> _configs;
    std::vector<pollfd> _pollfds;
    std::map<int , Server> _servers;
    std::map<int , Client> _clients;
    std::map<int , FD_info> _fdInfos;
    std::vector<std::string> _tokens;
public:
    WebServ();
    ~WebServ();
    WebServ(const WebServ &other);
    WebServ &operator=(const WebServ &other);
    void start();
    void setup();
    // process
    void newConnection(int server_fd);
    void readFromClient(int fd);
    void cgiProcess(int fd);
    void set_CgiRequirements(Client &client);
    void pollinprocess(int fd);
    void polloutprocess(int fd);
    void parsing(const std::string &filename);

    void child_process_cgi();
    void handleRequest(int fd);
    void addinfo(int fd, FD_type type, void *obj);
    void addpollfd(int fd, short events);
    void addserver(int fd, const Config &config);
    void addclient(int fd, const Config &config, std::vector<std::string> &tokens);
    void changePollToWrite(int fd) {
        for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
            if (it->fd == fd) {
                it->events = POLLOUT;
                break;
            }
        }
    }
    FD_type getFDType(int fd);
    // void displayConfigs() const;
    void removeClient(int fd) {
        _clients.erase(fd);
        _fdInfos.erase(fd);
        for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
            if (it->fd == fd) {
                _pollfds.erase(it);
                break;
            }
        }
        close(fd);
    }
};
