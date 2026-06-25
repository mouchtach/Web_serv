#include "LocationConfig.hpp"
#include <iostream>

const std::string &LocationConfig::getPath() const { return _path; }

const std::pair<int, std::string>& LocationConfig::getReturn() const { return _return; }

LocationConfig::LocationConfig() : _path("") {}

void LocationConfig::setPath(const std::string &path) { _path = path; }

void LocationConfig::setReturn(const std::vector<std::string>& tokens, size_t* i) {
//  std::cout << "token : " << tokens[*i] << std::endl;
    // _return.first = std::atoi(tokens[*i].c_str());
    // // if (_return.first < 300 || _return.first > 399)
    // //     throw std::runtime_error("Location: return code must be 3xx, got "+ tokens[*i]);
    // ++(*i);
    // if (*i >= tokens.size())
    //     throw std::runtime_error("Location: return missing URL after code "+ tokens[*i-1]);
    _return.second = stripSemicolon(tokens[*i]);
    if (_return.second.empty())
        throw std::runtime_error("Location: return URL is empty after code "+ tokens[*i-1]);
    ++(*i);
}

#include <iostream>
LocationConfig::~LocationConfig() {
}

void LocationConfig::print() const {
    std::cout << "Location Path: " << _path << std::endl;
    std::cout << "Root: " << _root << std::endl;
    std::cout << "Index: " << _index << std::endl;
    std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
    std::cout << "Client Max Body Size: " << _clientMaxBodySize << std::endl;
    std::cout << "Methods: ";
    for (size_t i = 0; i < _methods.size(); ++i)
        std::cout << _methods[i] << (i < _methods.size() - 1 ? ", " : "");
    std::cout << std::endl;
    std::cout << "CGI Extension: " << _cgiExtension << std::endl;
    std::cout << "CGI Path: " << _cgiPath << std::endl;
    // std::cout << "Return: " << _return << std::endl;
}

bool LocationConfig::isMethodAllowed(int method) const
{
    if (_methods.empty()) {
        return true; // If no methods specified, allow all methods
    }
    std::string methods[] = {"GET", "POST", "DELETE"};
    for (size_t i = 0; i < _methods.size(); ++i)
        if (_methods[i] == methods[method]) return true;
    return false;
}

// #include <iostream>
// void LocationConfig::PrintLocationConfig() const
// {
//     std::cout << "Location Path: " << _path << std::endl;
//     std::cout << "Root: " << _root << std::endl;
//     std::cout << "Index: " << _index << std::endl;
//     std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
//     std::cout << "Client Max Body Size: " << _clientMaxBodySize << std::endl;
//     std::cout << "Methods: ";
//     for (size_t i = 0; i < _methods.size(); ++i)
//         std::cout << _methods[i] << (i < _methods.size() - 1 ? ", " : "");
//     std::cout << std::endl;
//     std::cout << "CGI Extension: " << _cgiExtension << std::endl;
//     std::cout << "CGI Path: " << _cgiPath << std::endl;
//     std::cout << "Return: " << _return << std::endl;
// }
