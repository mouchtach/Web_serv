#include "LocationConfig.hpp"
#include <iostream>

const std::string &LocationConfig::getPath() const { return _path; }

const std::pair<int, std::string>& LocationConfig::getReturn() const { return _return; }

LocationConfig::LocationConfig() : _path("") {}

void LocationConfig::setPath(const std::string &path) { _path = path; }

void LocationConfig::setReturn(const std::vector<std::string>& tokens, size_t* i) {
    _return.first = std::atoi(tokens[*i].c_str());
    ++(*i);
    if (*i >= tokens.size())
        throw std::runtime_error("Location: return missing URL after code "+ tokens[*i-1]);
    _return.second = stripSemicolon(tokens[*i]);
    if (_return.second.empty())
        throw std::runtime_error("Location: return URL is empty after code "+ tokens[*i-1]);
    ++(*i);
    redc = 1;
}

#include <iostream>
LocationConfig::~LocationConfig() { }

void LocationConfig::print() const {
    std::cout  << "Location Path: " << _path << std::endl;
    std::cout  << "Root: " << _root << std::endl;
    std::cout  << "Index: " << _index << std::endl;
    std::cout  << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;
    std::cout  << "Client Max Body Size: " << _clientMaxBodySize << std::endl;
    std::cout  << "Methods: ";
    for (size_t i = 0; i < _methods.size(); ++i)
        std::cout  << _methods[i] << (i < _methods.size() - 1 ? ", " : "");
    std::cout  << std::endl;
    std::cout  << "CGI Extension: " << _cgiExtension << std::endl;
    std::cout  << "CGI Path: " << _cgiPath << std::endl;
    // std::cout  << "Return: " << _return << std::endl;
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

void LocationConfig::override(const std::vector<std::string> &tokens, size_t &i, const std::string path) {

  setPath(path);

  while (i < tokens.size() && tokens[i] != "}") {
    std::string directive = tokens[i++];
    if (directive == "root") {
      if (i >= tokens.size())
        error("location '" + path + "': 'root' missing value");
      setRoot(tokens[i++]);
      rootOverridden = true;
    } else if (directive == "index") {
      if (i >= tokens.size())
        error("location '" + path + "': 'index' missing value");
      setIndex(tokens[i++]);
    } else if (directive == "autoindex") {
      if (i >= tokens.size())
        error("location '" + path + "': 'autoindex' missing value");
     setAutoindex(tokens[i++]);
    } else if (directive == "methods") {
      if (i >= tokens.size())
        error("location '" + path + "': 'methods' missing values");
      std::vector<std::string> listmethods;
      while (i < tokens.size() && tokens[i] != "}") 
	  {
		std::string m = tokens[i++];
		listmethods.push_back(m);
		if (!m.empty() && m[m.size() - 1] == ';')
		  break;
      }
      setMethods(listmethods);

    } else if (directive == "cgi_extension") {
      if (i >= tokens.size())
        error("location '" + path + "': 'cgi_extension' missing value");
      setCgiExtension(tokens[i++]);
    } else if (directive == "cgi_path") {
      if (i >= tokens.size())
        error("location '" + path + "': 'cgi_path' missing value");
      setCgiPath(tokens[i++]);
    } else if (directive == "return") {
      if (i >= tokens.size())
        error("location '" + path + "': 'return' missing value");
      setReturn(tokens, &i);
    } else {
      error("Unknown directive in location '" + path + "': '" + directive + "'");
    }
  }
}
