#pragma once
#include "../parssing/config.hpp"

class Server {

private:
    Config _config;
public:
    Server();
    ~Server();
    Server(const Server &other);
    Server(const Config &config);
    Server &operator=(const Server &other);

    const Config &getConfig() const;
};