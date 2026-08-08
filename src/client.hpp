#pragma once
#include "redirectException.hpp"
#include "../parssing/config.hpp"
#include "../parssing/location.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"
#include "static_utils.hpp"
#include <iostream>
#include <cstdlib>

class Client {
private:

    Config _config;
    Request _request;
    Response _response;
    Location _matchedLocation;
    std::vector<std::string> *_tokens;


    int _fd;
    pid_t _cgiPid;
    int   _cgiOutFd;
    std::string _cgiOutput; 
    std::string _cgiBody;  
    size_t      _cgiBodySent;

public:

    Client();
    ~Client();
    Client(const Client &other);
    Client(const Config &config, std::vector<std::string> *tokens, int fd);
    Client &operator=(const Client &other);

// Getters 
    Request &getRequest();
    Response &getResponse();
    Location getMatchedLocation() const;
    const Config &getConfig() const ;
    const std::string &getCgiBody() const;
    const std::string &getCgiOutput() const;
    size_t getCgiBodySent() const;
    int  getCgiOutFd() const;
    pid_t getCgiPid() const ;
    int  getFd() const ;


// Setters

    void setCgiBody(const std::string &body);
    void setFd(int fd);
    void setCgiPid(pid_t p);
    void setCgiOutFd(int fd);

// METHODS
    void appendCgiOutput(const char *buf, size_t n);
    void addCgiBodySent(size_t n);
    void processStatic();
    void redirect(int code, const std::string &newLocation);
    void handleStaticGET(const std::string &target, const std::string &uri);
    void handleStaticPOST(const std::string &target);
    void handleStaticDELETE(const std::string &target);
    void sendFile(const std::string &filepath);
    void processAutoIndex(const std::string &uri, const std::string &target);
    void receiveBuffer(int fd);
    void matchLocation();
    void checkAccess();
    bool isMethodeAllowed();
    bool validateToken(std::string token);
};