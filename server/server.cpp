#include "server.hpp"
#include <sys/socket.h>
// #include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

Server::Server(const Config &config) : _config(config) {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_config.getPort());
    if (bind(_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(_fd, MAX_CANON) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout  << "Server listening on port " << _config.getPort() << std::endl;

}

Server::~Server() {
    if (_fd >= 0) {
        close(_fd);
    }
}
