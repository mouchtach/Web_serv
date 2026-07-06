#pragma once
#include "../config/Config.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"
#include <poll.h>

class LocationConfig;

class Client
{
private:
  int             _fd;
  int             _cgifd;
  int            _cgiPid;
  Config          _config;
  LocationConfig  _targetLocation;
  // i want pointer to point to the vector pollfds in webserv class to add the cgi fd to it when i fork a new process for cgi
  std::vector<pollfd>* _pollfds;
  
public:
  Client();
  Request         _request;
  Response        _response;
  Client(int fd, const Config &config);
  ~Client();

  // getters
  int getFd() const;
  const Config &getConfig() const;
  const Request &getRequest() const;
  const Response &getResponse() const;

  void setFdsPointer(std::vector<pollfd>& pollfds) { _pollfds = &pollfds; }

  void findTargetLocation();
  void processResponse();
  void processAutoIndex(const std::string &uri, const std::string &target);
  void redirection(int statuscode, const std::string &newLocation);
  void sendFile(const std::string &filepath);

  bool iscgi() const { return _cgifd != -1; }
  int getCgiFd() const { return _cgifd; }
  int getCgiPid() const { return _cgiPid; }
  // void setCgiFd(int fd) { _cgifd = fd; }
  void handelGET(std::string target, std::string uri);
  void handelPOST(std::string target);
  void handelDELETE(std::string& target);
  void handleCGI(std::string& target);
};