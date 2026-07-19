#pragma once

#include "../parssing/config.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"


class Client {
private:
    // token key for the client, can be used for authentication or session management

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

    ssize_t receivebuffer(int fd);

    Request &getRequest() {
        return _request;
    }
    Response &getResponse() {
        return _response;
    }
    
};