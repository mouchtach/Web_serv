#include "serverConfig.hpp"
// #include "LocationConfig.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>

// int serverConfig::getPort() const { return _port; }
// const std::string &serverConfig::getServerName() const { return _serverName; }
const std::string &serverConfig::getRoot() const { return _root; }
const std::string &serverConfig::getIndex() const { return _index; }
bool serverConfig::getAutoindex() const { return _autoindex; }
size_t serverConfig::getClientMaxBodySize() const {
  return _clientMaxBodySize;
}
// const std::vector<LocationConfig *> &serverConfig::getLocations() const {
//   return _locations;
// }
const std::map<int, std::string> &serverConfig::getErrorPages() const {
  return _errorPages;
}
const std::vector<std::string> &serverConfig::getMethods() const { return _methods; }
const std::string &serverConfig::getCgiExtension() const { return _cgiExtension; }
const std::string &serverConfig::getCgiPath() const { return _cgiPath; }
// const std::string &serverConfig::getReturn() const { return _return; }


serverConfig::serverConfig() {
//   _port = 0;
//   _serverName = "";
//   _root = "";
//   _index = "";
//   _autoindex = false;
//   _clientMaxBodySize = "0";
//   _errorPages.clear();
// //   _locations.clear();
//   _methods.clear();
//   _cgiExtension = "";
//   _cgiPath = "";
//   _return = "";
}
serverConfig::~serverConfig() {
  _errorPages.clear();
//   for (size_t i = 0; i < _locations.size(); ++i) {
//     delete _locations[i];
//   }
//   _locations.clear();
  _methods.clear();
  _cgiExtension.clear();
  _cgiPath.clear();
  // _return.second.clear();
}

// void serverConfig::setPort(const std::string &port) {
//   std::string p = stripSemicolon(port);
//   for (size_t i = 0; i < p.size(); ++i)
//     if (!isdigit(p[i]))
//       throw std::runtime_error("Server: port must be a number, got '" + p +
//                                "'");
//   int val = std::atoi(p.c_str());
//   if (val < 1 || val > 65535)
//     throw std::runtime_error("Server: port out of range (1-65535), got " + p);
//   _port = val;
//   // std::cout  << "Port set to: " << _port << std::endl;
// }

// void serverConfig::setServerName(const std::string &name) {
//   _serverName = stripSemicolon(name);
//   if (_serverName.empty())
//     throw std::runtime_error("Server: server_name value is empty");
// }

void serverConfig::setRoot(const std::string &root) {
  _root = stripSemicolon(root);
  if (_root.empty())
    throw std::runtime_error("Server: root value is empty");
}

void serverConfig::setIndex(const std::string &index) {
  _index = stripSemicolon(index);
  if (_index.empty())
    throw std::runtime_error("Server: index value is empty");
}
void serverConfig::setAutoindex(const std::string &autoindex) {
  std::string v = stripSemicolon(autoindex);
  if (v == "on")
    _autoindex = true;
  else if (v == "off")
    _autoindex = false;
  else
    throw std::runtime_error("Server: autoindex must be 'on' or 'off', got '" +
                             v + "'");
}

void serverConfig::setClientMaxBodySize(const std::string &size) {
  std::string s = stripSemicolon(size);
  for (size_t i = 0; i < s.size(); ++i)
    if (!isdigit(s[i]))
      throw std::runtime_error(
          "Server: client_max_body_size must be a number, got '" + s + "'");
  _clientMaxBodySize = std::strtoul(s.c_str(), NULL, 10);
  if (_clientMaxBodySize == 0)
    throw std::runtime_error(
        "Server: client_max_body_size must be greater than 0, got '" + s + "'");
}

void serverConfig::addErrorPage(const std::string &code, const std::string &path) {
  for (size_t i = 0; i < code.size(); ++i)
    if (!isdigit(code[i]))
      throw std::runtime_error(
          "Server: error_page code must be a number, got '" + code + "'");

  int c = std::atoi(code.c_str());
  if (c < 400 || c > 599)
    throw std::runtime_error(
        "Server: error_page code must be 4xx or 5xx, got " + code);

  std::string p = stripSemicolon(path);
  if (p.empty())
    throw std::runtime_error("Server: error_page path is empty for code " +
                             code);

  _errorPages[c] = p;
}

// void serverConfig::addLocation(const LocationConfig &loc) {
//   LocationConfig *newLoc = new LocationConfig(loc);
//   for (size_t i = 0; i < _locations.size(); ++i)
//     if (_locations[i]->getPath() == loc.getPath())
//       throw std::runtime_error("Server port " + intToStr(_port) +
//                                ": duplicate location '" + loc.getPath() + "'");
//   _locations.push_back(newLoc);
// }

void serverConfig::setMethods(const std::vector<std::string> &methods) {
  // _methods.clear();
  // // std::cout  << "methods test \n";
  for (size_t i = 0; i < methods.size(); ++i) {
    std::string m = methods[i];
    if (i == methods.size() - 1 && !m.empty()) {
      m = stripSemicolon(m);
    }
    if (m != "GET" && m != "POST" && m != "DELETE") {
      throw std::runtime_error("Server: invalid method '" + m + "'");
    }
    // // std::cout  << "Adding method: " << m << std::endl;
    _methods.push_back(m);
  }
}

void serverConfig::setCgiExtension(const std::string &cgiExtension) {
  _cgiExtension = stripSemicolon(cgiExtension);
}

void serverConfig::setCgiPath(const std::string &cgiPath) {
  _cgiPath = stripSemicolon(cgiPath);
}



std::string serverConfig::stripSemicolon(const std::string &s) {
  if (!s.empty() && s[s.size() - 1] == ';')
    return s.substr(0, s.size() - 1);
  else
    throw std::runtime_error("Value must end with ';', got '" + s + "'");
}

std::string serverConfig::intToStr(int n) {
  std::ostringstream ss;
  ss << n;
  return ss.str();
}
