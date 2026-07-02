#include "client.hpp"
#include "utils.h"
#include <sys/stat.h>
#include <unistd.h>

Client::Client(): _fd(-1), _config(), _targetLocation() , _request(), _response() {}

Client::Client(int fd, const Config &config) : _fd(fd), _config(config) {
    _response.setConfig(config);
}

Client::~Client() {
}

// getters
int Client::getFd() const { return _fd; }
const Config &Client::getConfig() const { return _config; }
const Response &Client::getResponse() const { return _response; }
const Request &Client::getRequest() const { return _request; }


void Client::findTargetLocation() {
    const std::vector<LocationConfig> &locations = _config.getLocations();
    std::string uri = _request.getUri();

    for (size_t i = 0; i < locations.size(); i++) 
    {
        if (isMatch(uri, locations[i].getPath())) 
        {
            if (_targetLocation.getPath().empty() || locations[i].getPath().size() > _targetLocation.getPath().size()) 
                _targetLocation = locations[i];
        }
    }
}

void Client::processResponse() {
    if (_request.getMethod() == GET) {
        handelGET();
    } else if (_request.getMethod() == POST) {
        handelPOST();
    } else if (_request.getMethod() == DELETE) {
        handelDELETE();
    }
}

void Client::processAutoIndex(const std::string &uri, const std::string &target) {
    std::string autoIndexHtml = buildAutoIndex(target, uri);
    if (autoIndexHtml.empty()) {
        _response.sendError(403);
        return;
    }
    _response.setStatusCode("200");
    _response.setversion("HTTP/1.0");
    _response.setStatusMessage("OK");
    _response.setHeader("Content-Length", std::to_string(autoIndexHtml.size()));
    _response.setHeader("Content-Type", "text/html");
    _response.setBody(autoIndexHtml);
    _response.buildResponse();
}

void Client::redirection(int statuscode, const std::string &newLocation) {
  _response.setStatusCode(std::to_string(statuscode));
  _response.setversion("HTTP/1.0");
  _response.setStatusMessage("Moved Permanently");
  _response.setHeader("Location", newLocation);
  _response.setHeader("Content-Length", "0");
  _response.buildResponse();
}

void Client::sendFile(const std::string &filepath) {
  std::string content = readFile(filepath);
  if (content.empty()) {
    _response.sendError(404);
    return;
  }
  _response.setStatusCode("200");
  _response.setversion("HTTP/1.0");
  _response.setStatusMessage("OK");
  _response.setHeader("Content-Length", std::to_string(content.size()));
  _response.setHeader("Content-Type", getMimeType(filepath));
  _response.setBody(content);
  _response.buildResponse();
}


void Client::handelDELETE() {
  findTargetLocation();
  std::string uri = _request.getUri();
  if (isPathsafe(uri) == false)
  {
      _response.sendError(403);
      return;
  }
  if(_targetLocation.isMethodAllowed(DELETE) == false)
  {
      std::cout << "DELETE method not allowed for this location" << std::endl;
      _response.sendError(405);
      return;
  }
  std::string root = _targetLocation.getRoot();
  std::string locationPath = _targetLocation.getPath();
  std::string pathToAppend;

  if (_targetLocation.isRootOverridden())
  {
      std::string uriSuffix = uri.substr(locationPath.length());
      pathToAppend = uriSuffix;
  }
  else
  {
      pathToAppend = uri;
  }

  std::string target = appendPath(root, pathToAppend);
  struct stat statBuf;
  if (stat(target.c_str(), &statBuf) == -1) {
    _response.sendError(404);
    return;
  }
  
  if (remove(target.c_str()) != 0) {
    _response.sendError(500);
    return;
  }

  _response.setStatusCode("200");
  _response.setversion("HTTP/1.0");
  _response.setStatusMessage("OK");
  _response.buildResponse();
}

void Client::handelPOST(){
  
  findTargetLocation();
  std::string uri = _request.getUri();
  if (isPathsafe(uri) == false)
  {
      _response.sendError(403);
      return;
  }
  if(_targetLocation.isMethodAllowed(POST) == false)
  {
      std::cout << "POST method not allowed for this location" << std::endl;
      _response.sendError(405);
      return;
  }
  std::string root = _targetLocation.getRoot();
  std::string locationPath = _targetLocation.getPath();
  std::cout << "POST uri: " << uri << std::endl;
  std::string pathToAppend;

  if (_targetLocation.isRootOverridden())
  {
      std::string uriSuffix = uri.substr(locationPath.length());
      pathToAppend = uriSuffix;
  }
  else
  {
      pathToAppend = uri;
  }

  std::cout << "pathToAppend: " << pathToAppend << std::endl;
  std::string target = appendPath(root, pathToAppend);
  std::cout << "POST target: " << target << std::endl;
  struct stat statBuf;
  if (stat(target.c_str(), &statBuf) == -1) {
    _response.sendError(404);
    return;
  }
  std::string fileName = extractFileName(_request.getBody());
  std::cout << "POST fileName: " << fileName << std::endl;

  // creat file
  std::string filePath = appendPath(target, fileName);
  std::ofstream outFile(filePath, std::ios::binary);
  if (!outFile) {
    _response.sendError(500);
    return;
  }

  
  outFile << extractBodyfile(_request.getBody());
  outFile.close();
  std::cout << "POST file created: " << filePath << std::endl;

  _response.setStatusCode("200");
  _response.setversion("HTTP/1.0");
  _response.setStatusMessage("OK");
  _response.buildResponse();
  // (getFd());
}

void Client::handelGET() {

  findTargetLocation();
  std::string uri = _request.getUri();
  // std::cout << "GET uri: " << uri << std::endl;
  std::string root = _targetLocation.getRoot();
  std::string index = _targetLocation.getIndex();
  std::string locationPath = _targetLocation.getPath();
  std::string uriSuffix;
  std::string pathToAppend;

  if(isPathsafe(uri) == false)
  {
      _response.sendError(403);
      return;
  }
  if(_targetLocation.hasredirection()){
    redirection(_targetLocation.getReturn().first, root + _targetLocation.getReturn().second);
    return;
  }
  if (_targetLocation.isRootOverridden())
  {
      uriSuffix = uri.substr(locationPath.length());
      pathToAppend = uriSuffix;
  }
  else
  {
      pathToAppend = uri;
  }
  std::string target = appendPath(root, pathToAppend);
  struct stat statBuf;
  if (stat(target.c_str(), &statBuf) == -1) {
    _response.sendError(404);
    return;
  }
  if (S_ISREG(statBuf.st_mode)) {
      if(_targetLocation.isMethodAllowed(GET))
        sendFile(target);
      else 
        _response.sendError(405);
      return;

  } else if (S_ISDIR(statBuf.st_mode)) {
    if (uri[uri.length() - 1] != '/') {
      redirection(301, uri + "/");
      return;
    }
    if(!_targetLocation.isMethodAllowed(GET))
    {
        _response.sendError(405);
        return;
    }
    std::string indexPath = target + "/" + index;
    struct stat indexStat;
    if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
      sendFile(indexPath);
      return;
    } else {
      if (_targetLocation.getAutoindex()) {
        processAutoIndex(uri, target);
      } else {
        _response.sendError(403);
        return;
      }
    }
  } else {
    _response.sendError(403);
    return;
  }
}
