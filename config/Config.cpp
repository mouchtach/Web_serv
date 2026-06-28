#include "Config.hpp"
#include "LocationConfig.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>

int Config::getPort() const { return _port; }

const std::string &Config::getServerName() const { return _serverName; }

const std::vector<LocationConfig>& Config::getLocations() const { return _locations; }

Config::Config() {
  _port = 0;
  _serverName = "";
  _locations.clear();
}
Config::~Config() {
    _locations.clear();
}

void Config::setPort(const std::string &port) {
  std::string p = stripSemicolon(port);
  for (size_t i = 0; i < p.size(); ++i)
    if (!isdigit(p[i]))
      throw std::runtime_error("Server: port must be a number, got '" + p +"'");
  int val = std::atoi(p.c_str());
  if (val < 1 || val > 65535)
    throw std::runtime_error("Server: port out of range (1-65535), got " + p);
  _port = val;
}

void Config::setServerName(const std::string &name) {
  _serverName = stripSemicolon(name);
  if (_serverName.empty())
    throw std::runtime_error("Server: server_name value is empty");
}

void Config::addLocation(const LocationConfig& location) {
    for (size_t i = 0; i < _locations.size(); ++i) 
    {
        if (_locations[i].getPath() == location.getPath()) {
          throw std::runtime_error("Server port " + intToStr(_port) + ": duplicate location '" + location.getPath() + "'");
        }
    }
    _locations.push_back(location);
}
