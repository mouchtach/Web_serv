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
  Config          _config;
  LocationConfig  _targetLocation;
  
  
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


  void findTargetLocation();
  void processResponse();
  void processAutoIndex(const std::string &uri, const std::string &target);
  void redirection(int statuscode, const std::string &newLocation);
  void sendFile(const std::string &filepath);

  void handelGET();
  void handelPOST();
  void handelDELETE();
};