#pragma once

#include "../parssing/config.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"
#include <iostream>

class Client {
private:

    Config _config;
    Request _request;
    Response _response;
    std::string _token;

public:
    Client();
    ~Client();
    Client(const Client &other);
    Client(const Config &config);
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

    void handleRequest() {
        // display request information
        std::cout << "Received request:" << std::endl;
        std::cout << "Method: " << _request.getMethod() << std::endl;
        std::cout << "URI: " << _request.getUri() << std::endl;
        std::cout << "Headers:" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = _request.getHeaders().begin(); it != _request.getHeaders().end(); ++it) {
            std::cout << it->first << ": " << it->second << std::endl;
        }
        std::cout << "Body: " << _request.getBody() << std::endl;
        // Here you would implement the logic to handle the request and generate a response
        // For now, we just set a simple response
        // _response.setBuffer("HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");
        exit(0);
    }
    // bool hascontentlength() const {
    //     return _config.getClientMaxBodySize() > 0;
    // }
};