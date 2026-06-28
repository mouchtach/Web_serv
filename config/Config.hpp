#pragma once

#include "serverConfig.hpp"
#include "LocationConfig.hpp"

class Config : public serverConfig
{
private:
    int _port;
    std::string _serverName;
    std::vector<LocationConfig> _locations;

public:
    Config();
    virtual ~Config();
    void setPort(const std::string& port);
    void setServerName(const std::string& name);
    void addLocation(const LocationConfig& location);

    int getPort() const;
    const std::string& getServerName() const;
    const std::vector<LocationConfig>& getLocations() const;
};