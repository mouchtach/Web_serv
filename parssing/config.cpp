#include "config.hpp"
#include "location.hpp"
#include "utils.hpp"
#include <stdexcept>

Config::Config() : _port(0), _autoindex(false), _clientMaxBodySize(0) {}

Config::~Config() {}

Config::Config(const Config &other) : _port(other._port), _serverName(other._serverName), _locations(other._locations), _root(other._root), _index(other._index), _autoindex(other._autoindex), _clientMaxBodySize(other._clientMaxBodySize), _errorPages(other._errorPages), _methods(other._methods) {}

Config &Config::operator=(const Config &other) {
	if (this != &other) {
		_port = other._port;
		_serverName = other._serverName;
		_locations = other._locations;
		_root = other._root;
		_index = other._index;
		_autoindex = other._autoindex;
		_clientMaxBodySize = other._clientMaxBodySize;
		_errorPages = other._errorPages;
		_methods = other._methods;
		// _hasmaxbodysize = other._hasmaxbodysize;
	}
	return *this;
}

// Setters

void Config::setPort(const std::string &portStr) {
    std::string p = stripSemicolon(portStr);
    for (size_t i = 0; i < p.size(); ++i)
        if (!isdigit(p[i]))
            throw std::runtime_error("Server: port must be a number, got '" + p +"'");
    int val = std::atoi(p.c_str());
    if (val < 1 || val > 65535)
        throw std::runtime_error("Server: port out of range (1-65535), got " + p);
    _port = val;
}

void Config::setServerName(const std::string &serverName) {
	_serverName = stripSemicolon(serverName);
    if (_serverName.empty())
        throw std::runtime_error("Server: server_name value is empty");
}

void Config::setRoot(const std::string &root) {
	_root = stripSemicolon(root);
}

void Config::setIndex(const std::string &index) {
	_index = stripSemicolon(index);
}

void Config::setAutoindex(const std::string &autoindex) {
    std::string ai = stripSemicolon(autoindex);
	if (ai == "on")
		_autoindex = true;
	else if (ai == "off")
		_autoindex = false;
	else
		throw std::invalid_argument("Invalid value for autoindex: " + autoindex);
}

void Config::setClientMaxBodySize(const std::string &sizeStr) {
    std::string s = stripSemicolon(sizeStr);
    for (size_t i = 0; i < s.size(); ++i)
        if (!isdigit(s[i]))
            throw std::runtime_error("Server: client_max_body_size must be a number, got '" + s + "'");
    _clientMaxBodySize = std::strtoul(s.c_str(), NULL, 10);
    if (_clientMaxBodySize == 0)
        throw std::runtime_error("Server: client_max_body_size must be greater than 0, got '" + s + "'");
}

void Config::setMethods(const std::vector<std::string> &methods) {
    _methods.clear();
    for (size_t i = 0; i < methods.size(); ++i) {
        std::string m = methods[i];
        if (i == methods.size() - 1 && !m.empty()) {
            m = stripSemicolon(m);
        }
        if (m != "GET" && m != "POST" && m != "DELETE") {
            throw std::runtime_error("Server: invalid method '" + m + "'");
        }
        _methods.push_back(m);
    }
}


void Config::addErrorPage(const std::string &codeStr, const std::string &path) {
    for (size_t i = 0; i < codeStr.size(); ++i)
        if (!isdigit(codeStr[i]))
            throw std::runtime_error("Server: error_page code must be a number, got '" + codeStr + "'");
    int c = std::atoi(codeStr.c_str());
    if (c < 400 || c > 599)
        throw std::runtime_error("Server: error_page code must be 4xx or 5xx, got " + codeStr);
    std::string p = stripSemicolon(path);
    if (p.empty())
        throw std::runtime_error("Server: error_page path is empty for code " + codeStr);
    _errorPages[c] = p;
}

void Config::addLocation(const Location &location) {
	_locations.push_back(location);
}


// Getters

int Config::getPort() const {return _port;}
const std::string &Config::getServerName() const {return _serverName;}
const std::string &Config::getRoot() const {return _root;}
const std::string &Config::getIndex() const {return _index;}
bool Config::getAutoindex() const {return _autoindex;}
size_t Config::getClientMaxBodySize() const {return _clientMaxBodySize;}
const std::map<int, std::string> &Config::getErrorPages() const {return _errorPages;}
const std::vector<Location> &Config::getLocations() const {return _locations;}
const std::vector<std::string> &Config::getMethods() const {return _methods;}
