#pragma once

#include <string>
#include <vector>
#include <map>

class Location;

class Config{
private:
    int                     _port;
    std::string             _serverName;
    std::vector<Location>   _locations;
protected:    
    std::string                 _root;
    std::string                 _index;
    bool                        _autoindex;
    size_t                      _clientMaxBodySize;
    std::map<int, std::string>  _errorPages;
    std::vector<std::string>    _methods;

public:
    Config();
    ~Config();
    Config(const Config &other);
    Config &operator=(const Config &other);
    // Setters
    void setPort(const std::string &portStr);
    void setServerName(const std::string &serverName);
    void setRoot(const std::string &root);
    void setIndex(const std::string &index);
    void setAutoindex(const std::string &autoindex);
    void setClientMaxBodySize(const std::string &sizeStr);
    void addErrorPage(const std::string &codeStr, const std::string &path);
    void addLocation(const Location &location);
    void setMethods(const std::vector<std::string> &methods);
    // void addMethod(const std::string &method);
    // Getters
    int getPort() const;
    const std::string &getServerName() const;
    const std::string &getRoot() const;
    const std::string &getIndex() const;
    bool getAutoindex() const;
    size_t getClientMaxBodySize() const;
    const std::map<int, std::string> &getErrorPages() const;
    const std::vector<Location> &getLocations() const;
    const std::vector<std::string> &getMethods() const;
};

