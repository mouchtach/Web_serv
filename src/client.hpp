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

public:
    Client();
    ~Client();
    Client(const Client &other);
    Client(const Config &config, const std::vector<std::string> &tokens);
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
        if (_matchedLocation.getPath() == "/sigup" && validateToken(_request.getToken())) {
            throw redirectException("/home");
        } else if (_matchedLocation.getPath() == "/login" && !validateToken(_request.getToken())) {
            throw redirectException("/login");
        } else if (_matchedLocation.getPath() == "/sigup" && !validateToken(_request.getToken())) {
            std::cout << "Access granted to /sigup" << std::endl;
        } else if(!validateToken(_request.getToken())) {
            throw redirectException("/login");
        } else {
            std::cout << "Access granted to " << locationPath << std::endl;
        }
    }


    // bool hascontentlength() const {
    //     return _config.getClientMaxBodySize() > 0;
    // }
};