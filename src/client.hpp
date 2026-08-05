#pragma once

#include "redirectException.hpp"
#include "../parssing/config.hpp"
#include "../parssing/location.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"
#include <iostream>
#include <cstdlib>

class Client {
private:

    Config _config;
    Request _request;
    Response _response;
    Location _matchedLocation;
    std::vector<std::string> _tokens;

    int _fd;                 // the client's own socket fd (needed to route back)
    pid_t _cgiPid;
    int   _cgiOutFd;         // read end, registered with poll
    std::string _cgiOutput;  // accumulated raw CGI output

public:
    void setFd(int fd) { _fd = fd; }
    int  getFd() const { return _fd; }
    void setCgiPid(pid_t p) { _cgiPid = p; }
    pid_t getCgiPid() const { return _cgiPid; }
    void setCgiOutFd(int fd) { _cgiOutFd = fd; }
    int  getCgiOutFd() const { return _cgiOutFd; }
    void appendCgiOutput(const char *buf, size_t n) { _cgiOutput.append(buf, n); }
    const std::string &getCgiOutput() const { return _cgiOutput; }

public:
    Client();
    ~Client();
    Client(const Client &other);
    Client(const Config &config, const std::vector<std::string> &tokens, int fd);
    Client &operator=(const Client &other);

    void receiveBuffer(int fd);

    Request &getRequest() {
        return _request;
    }
    Response &getResponse() {
        return _response;
    }
    const Config &getConfig() const {
        return _config;
    }

    bool validateToken(std::string token);

    void matchLocation() {
        const std::vector<Location> &locations = _config.getLocations();
        const std::string &requestUri = _request.getUri();
        for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
            const Location &location = *it;
            const std::string &locationPath = location.getPath();
            if (requestUri.compare(0, locationPath.length(), locationPath) == 0) {
                _matchedLocation = location;
            }
        }
    }

    Location getMatchedLocation() {
        return _matchedLocation;
    }
    

    bool isMethodeAllowed() {
        const std::vector<std::string> &allowedMethods = _matchedLocation.getMethods();
        if (allowedMethods.empty()) {
            return true; // If no methods are specified, allow all methods
        }
        const std::string &requestMethod = _request.getMethod();
        for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
            if (*it == requestMethod) {
                return true;
            }
        }
        return false; // Method not allowed
    }

    void checkAccess() {
        
        const std::string &locationPath = _matchedLocation.getPath();
        std::cout << "Checking access for location: " << locationPath << std::endl;
        if ((_matchedLocation.getPath() == "/sigup" || _matchedLocation.getPath() == "/login") && validateToken(_request.getToken())) {
            std::cout << "should redirect to /home" << std::endl;
            throw redirectException("/home");
        } else if (_matchedLocation.getPath() == "/login" && !validateToken(_request.getToken())) {
            std::cout << "should redirect to /login" << std::endl;
            throw redirectException("/login");
            // std::cout << "Access granted to /login" << std::endl;
        } else if (_matchedLocation.getPath() == "/sigup" && !validateToken(_request.getToken())) {
            // std::cout << "should redirect to /sigup" << std::endl;
            // throw redirectException("/sigup");
            std::cout << "Access granted to /sigup" << std::endl;
        } else if(!validateToken(_request.getToken())) {
            throw redirectException("/login");
        } else {
            std::cout << "Access granted to " << locationPath << std::endl;
        }
    }

public:
    void processStatic();
    void redirect(int code, const std::string &newLocation);

private:
    void handleStaticGET(const std::string &target, const std::string &uri);
    void handleStaticPOST(const std::string &target);
    void handleStaticDELETE(const std::string &target);
    void sendFile(const std::string &filepath);
    void processAutoIndex(const std::string &uri, const std::string &target);
    // void redirect(int code, const std::string &newLocation);
};