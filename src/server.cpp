#include "server.hpp"

Server::Server() {
}

Server::~Server() {
}

Server::Server(const Server &other) : _config(other._config) {
}

Server::Server(const Config &config) : _config(config) {
}

Server &Server::operator=(const Server &other) {
    if (this != &other)
        _config = other._config;
    return *this;
}

