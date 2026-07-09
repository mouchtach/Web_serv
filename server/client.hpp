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
  // int             _cgifd;
  int            _cgiPid;
  Config          _config;
  LocationConfig  _targetLocation;
  std::string      _cgiBuffer;
  // std::string _cgiBody;       // body to send to CGI stdin
  // size_t      _cgiBodySent;   // how much of it we've written so far
  // std::string _cgiOutBuffer;
  // i want pointer to point to the vector pollfds in webserv class to add the cgi fd to it when i fork a new process for cgi
  std::vector<pollfd>* _pollfds;
  // std::vector< std::pair<int, int> > _cgiPipes; // pair of pipes for cgi process
  int _cgi_inputfd; // pipe for cgi process input
  int _cgi_outputfd; // pipe for cgi process output
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

  void appendCgiBuffer(const std::string& data) { _cgiBuffer.append(data); }
  const std::string& getCgiBuffer() const { return _cgiBuffer; }
  void findTargetLocation();
  void processResponse();
  void processAutoIndex(const std::string &uri, const std::string &target);
  void redirection(int statuscode, const std::string &newLocation);
  void sendFile(const std::string &filepath);

  // bool iscgi() const { return _cgifd != -1; }
  // bool isPipeCgi_input(int fd) const {
  //   for (size_t i = 0; i < _cgiPipes.size(); ++i) {
  //       if (_cgiPipes[i].first == 0 && _cgiPipes[i].second == fd) {
  //           return true;
  //       }
  //   }
  //   return false;
  // }
  // bool isPipeCgi_output(int fd) const {
  //   for (size_t i = 0; i < _cgiPipes.size(); ++i) {
  //       if (_cgiPipes[i].first == 1 && _cgiPipes[i].second == fd) {
  //           return true;
  //       }
  //   }
  //   return false;
  // }
  int getCgi_inputfd() const { 
    std::cout << "Getting CGI input fd: " << _cgi_inputfd << std::endl;
    return _cgi_inputfd; 
  }
  int getCgi_outputfd() const { 
    std::cout << "Getting CGI output fd: " << _cgi_outputfd << std::endl;
    return _cgi_outputfd; 
  }
  int getCgiPid() const { return _cgiPid; }
  void setCgiPid(int pid) { _cgiPid = pid; }
  // void setCgiFd(int fd) { _cgifd = fd; }
  void handelGET(std::string target, std::string uri);
  void handelPOST(std::string target);
  void handelDELETE(std::string& target);
  void handleCGI(std::string& target);

  // void setCgiBody(const std::string& body) { _cgiBody = body; _cgiBodySent = 0; }
  // bool cgiBodyFullySent() const { return _cgiBodySent >= _cgiBody.size(); }
  // size_t writeCgiBodyChunk(int fd){
  //   if (cgiBodyFullySent()) return 0;
  //   size_t remaining = _cgiBody.size() - _cgiBodySent;
  //   ssize_t n = write(fd, _cgiBody.c_str() + _cgiBodySent, remaining);
  //   if (n > 0) _cgiBodySent += n;
  //   return n > 0 ? (size_t)n : 0;
  // } // writes what it can, returns bytes written
  // void appendCgiOutput(const std::string& s) { _cgiOutBuffer += s; }
  // const std::string& getCgiOutput() const { return _cgiOutBuffer; }
};