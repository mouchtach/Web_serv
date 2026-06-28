#pragma once
#include "../config/Config.hpp"
#include "../http/request.hpp"
#include "../http/response.hpp"
#include <poll.h>
// #include <bool.h>

class LocationConfig;

class Client : public Request, public Response
{
private:

  int             _fd;
  Config          _config;
  Request         _request;
  Response        _response;
  LocationConfig  _targetLocation;

public:
  Client(){}
  Client(int fd, const Config &config);
  int getFd() const;
  const Config &getConfig() const;
  void findTargetLocation();

  // void setTargetPath();
  Request &getrequest() { return _request; }
  // void readRequest();
  void setResponse(const std::string &response) {
    _response.setRawResponse(response);
  }
  const Response &getResponse() const { return _response; }
  void processResponse();
  void redirection(int statuscode, const std::string &newLocation);
  void handelGET();
  void handelPOST() {
    // Implement POST request handling logic here
  }
  void handelDELETE() {
    // Implement DELETE request handling logic here
  }
  std::string getfilepath();
  void sendFile(const std::string &filepath);
  std::string buildAutoIndex(const std::string &dirPath, const std::string &uri);
};