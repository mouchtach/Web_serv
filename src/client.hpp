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

    void matchLocation()
    {
        const std::vector<Location> &locations = _config.getLocations();
        const std::string &requestUri = _request.getUri();
        std::cout << "Matching request URI: " << requestUri << std::endl;
        const Location *bestMatch = NULL;
        size_t bestLength = 0;

        for (std::vector<Location>::const_iterator it = locations.begin();
            it != locations.end(); ++it)
        {
            const std::string &locationPath = it->getPath();

            // URI must start with the location path
            if (requestUri.compare(0, locationPath.length(), locationPath) != 0)
                continue;

            // Make sure it is a complete path component
            bool valid = false;

            if (requestUri.length() == locationPath.length())
                valid = true;                         // "/login"
            else if (locationPath == "/")
                valid = true;                         // root matches everything
            else if (requestUri[locationPath.length()] == '/')
                valid = true;                         // "/login/user"

            if (!valid)
                continue;

            // Keep the longest matching location
            if (locationPath.length() > bestLength)
            {
                bestMatch = &(*it);
                bestLength = locationPath.length();
            }
        }

        if (bestMatch)
            _matchedLocation = *bestMatch;
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
        if (_request.getUri() == "/login.html" || _request.getUri() == "/signup.html" || locationPath == "/static") {
            return; // Allow access to login, signup, and static resources without token
        }
        // std::cout << "Checking access for location: " << locationPath << std::endl;
        if ((_matchedLocation.getPath() == "/signup" || _matchedLocation.getPath() == "/login" ) && validateToken(_request.getToken())) {
            // std::cout << "should redirect to /index.html" << std::endl;
            throw redirectException("/index.html");
        }
        // any path without token should redirect to /login
        else if (_matchedLocation.getPath() != "/signup" && _matchedLocation.getPath() != "/login" && !validateToken(_request.getToken())) {
            // std::cout << "should redirect to /login.html" << std::endl;
            // sendfile("./cgi/login.html");
            throw redirectException("/login.html");
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