#include "location.hpp"
#include "../src/static_utils.hpp"

Location::Location() : rootOverridden(false), redc(false) {}
Location::~Location() {}
Location::Location(const Location &other) : Config(other), _path(other._path), _targetPath(other._targetPath), rootOverridden(other.rootOverridden), redc(other.redc), _return(other._return), _cgiExtension(other._cgiExtension), _cgiPath(other._cgiPath) {}
Location::Location(const Config &other) : Config(other), rootOverridden(false), redc(false) {}

Location &Location::operator=(const Location &other) {
    if (this != &other) {
        Config::operator=(other);
        _path = other._path;
        _targetPath = other._targetPath;
        rootOverridden = other.rootOverridden;
        redc = other.redc;
        _return = other._return;
        _cgiExtension = other._cgiExtension;
        _cgiPath = other._cgiPath;
    }
    return *this;
}
void Location::setPath(const std::string &path) {_path = path;}
const std::string &Location::getPath() const {return _path;}
void Location::setTargetPath(const std::string &targetPath) {_targetPath = targetPath;}
const std::string &Location::getTargetPath() const {return _targetPath;}
void Location::setRootOverridden(bool overridden) {rootOverridden = overridden;}
bool Location::isRootOverridden() const {return rootOverridden;}
void Location::setRedc(bool redc) {this->redc = redc;}
bool Location::isRedc() const {return redc;}
void Location::setReturn(const std::pair<int, std::string> &ret) {_return = ret;}
const std::pair<int, std::string> &Location::getReturn() const {return _return;}
void Location::setCgiExtension(const std::string &cgiExtension) {_cgiExtension = stripSemicolon(cgiExtension);}
const std::string &Location::getCgiExtension() const {return _cgiExtension;}
void Location::setCgiPath(const std::string &cgiPath) {_cgiPath = stripSemicolon(cgiPath);}
const std::string &Location::getCgiPath() const {return _cgiPath;}

void Location::overrideLocation(const std::vector<std::string> &tokens, size_t &i) {
  while (i < tokens.size() && tokens[i] != "}") {
    std::string directive = tokens[i++];
    if (directive == "root") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'root' missing value");
      setRoot(tokens[i++]);
      rootOverridden = true;
    } else if (directive == "index") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'index' missing value");
      setIndex(tokens[i++]);
    } else if (directive == "autoindex") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'autoindex' missing value");
     setAutoindex(tokens[i++]);
    } else if (directive == "methods") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'methods' missing values");
      setMethods(parseMethodsList(tokens, i));
    } else if (directive == "cgi_extension") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'cgi_extension' missing value");
      setCgiExtension(tokens[i++]);
    } else if (directive == "cgi_path") {
      if (i >= tokens.size())
        throw std::runtime_error("location '" + _path + "': 'cgi_path' missing value");
      setCgiPath(tokens[i++]);
    } else if (directive == "return") {
      if (i >= tokens.size()) {
        throw std::runtime_error("location '" + _path + "': 'return' missing value");
      }
      std::pair<int, std::string> ret;
      ret.first = strToInt(tokens[i++]);
      if (ret.first < 300 || ret.first > 308) {
        throw std::runtime_error("location '" + _path + "': 'return' code must be a redirect status (300-308), got " + intToStr(ret.first));
      }
      if (i >= tokens.size()) {
        throw std::runtime_error("location '" + _path + "': 'return' missing URL");
      }
      ret.second = stripSemicolon(tokens[i++]);

      if (ret.second.empty()) {
        throw std::runtime_error("location '" + _path + "': 'return' URL is empty");
      }
      setReturn(ret);
      setRedc(true);
    } else {
      throw std::runtime_error("location '" + _path + "': Unknown directive '" + directive + "'");
    }
  }
}