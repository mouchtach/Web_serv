
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
    CGI_OUT,
    CGI_IN
};

typedef struct FD_info {
    int fd;
    FD_type type;
    void *obj;
} FD_info;

struct CgiResult {
    int statusCode;
    std::string statusMsg;
    std::map<std::string, std::string> headers;
    std::string body;
};

class WebServ {
private:
    std::vector<Config> _configs;
    std::vector<pollfd> _pollfds;
    std::map<int , Server> _servers;
    std::map<int , Client> _clients;
    std::map<int , FD_info> _fdInfos;
    std::vector<std::string> _tokens;
    CgiResult _result;

    void parseCgiOutput(const std::string &raw);
    void storeCgiToken(Client &client);
    void buildCgiResponse(Client &client);
    WebServ(const WebServ &other);
    WebServ &operator=(const WebServ &other);

public:
    WebServ();
    ~WebServ();
    void start();
    void setup();
    // process
    void newConnection(int server_fd);
    void readFromClient(int fd);
    void pollinprocess(int fd);
    void polloutprocess(int fd);
    void parsing(const std::string &filename);

    void cgiReadOutput(int fd); 
    void cgiWriteBody(int fd); 
    void handleRequest(int fd);
    void addinfo(int fd, FD_type type, void *obj);
    void addpollfd(int fd, short events);
    void addserver(int fd, const Config &config);
    void addclient(int fd, const Config &config, std::vector<std::string> &tokens);
    void closeFd(int fd);
    void removeCgiFd(int fd);
    void changePollToWrite(int fd);
    FD_type getFDType(int fd);
    void removeClient(int fd);
    void startCgi(int client_fd);
    std::vector<std::string> buildCgiEnv(Client &client, const std::string &scriptPath);
    void finalizeCgiResponse(Client &client);
    void loadTokens(const std::string &filename);
};
