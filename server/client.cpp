#include "client.hpp"
#include "utils.h"
#include <sys/stat.h>

Client::Client() : _fd(-1), _config(), _request(), _response(), _targetLocation() {}

Client::Client(int fd, const Config &config) : _fd(fd), _config(config) {
    _complete = false;
    setConfig(config);
}

// getters
int Client::getFd() const { return _fd; }
const Config &Client::getConfig() const { return _config; }
const Response &Client::getResponse() const { return _response; }
const Request &Client::getRequest() const { return _request; }


void Client::findTargetLocation() {
    const std::vector<LocationConfig> &locations = _config.getLocations();
    std::string uri = getUri();

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
    if (Request::getMethod() == GET) {
        handelGET();
    } else if (Request::getMethod() == POST) {
        handelPOST();
    } else if (Request::getMethod() == DELETE) {
        handelDELETE();
    }
}

void Client::processAutoIndex(const std::string &uri, const std::string &target) {
    std::string autoIndexHtml = buildAutoIndex(target, uri);
    if (autoIndexHtml.empty()) {
        sendError(403);
        return;
    }
    setStatusCode("200");
    setversion("HTTP/1.0");
    setStatusMessage("OK");
    setHeader("Content-Length", std::to_string(autoIndexHtml.size()));
    setHeader("Content-Type", "text/html");
    setBody(autoIndexHtml);
    buildResponse();
}

void Client::redirection(int statuscode, const std::string &newLocation) {
  setStatusCode(std::to_string(statuscode));
  setversion("HTTP/1.0");
  setStatusMessage("Moved Permanently");
  setHeader("Location", newLocation);
  setHeader("Content-Length", "0");
  buildResponse();
}

void Client::sendFile(const std::string &filepath) {
  std::string content = readFile(filepath);
  std::cout << "file size: " << content.size() << std::endl;
  if (content.empty()) {
    sendError(404);
    return;
  }
//   std::cout << "content length: " << content.size() << std::endl;
  setStatusCode("200");
  setversion("HTTP/1.0");
  setStatusMessage("OK");
  setHeader("Content-Length", std::to_string(content.size()));
  setHeader("Content-Type", getMimeType(filepath));
  setBody(content);
  buildResponse();
}


void Client::handelDELETE() {
  findTargetLocation();
  if(_targetLocation.isMethodAllowed(DELETE) == false)
  {
      std::cout << "DELETE method not allowed for this location" << std::endl;
      sendError(405);
      return;
  }
  std::string root = _targetLocation.getRoot();
  std::string locationPath = _targetLocation.getPath();
  std::string uri = getUri();
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
    sendError(404);
    return;
  }
  
  if (remove(target.c_str()) != 0) {
    sendError(500);
    return;
  }

  setStatusCode("200");
  setversion("HTTP/1.0");
  setStatusMessage("OK");
  buildResponse();
}

void Client::handelPOST(){
  
  findTargetLocation();
  if(_targetLocation.isMethodAllowed(POST) == false)
  {
      std::cout << "POST method not allowed for this location" << std::endl;
      sendError(405);
      return;
  }
  std::string root = _targetLocation.getRoot();
  std::string locationPath = _targetLocation.getPath();
  std::string uri = getUri();
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
    sendError(404);
    return;
  }
  std::string fileName = extractFileName(getBody());
  std::cout << "POST fileName: " << fileName << std::endl;

  // creat file
  std::string filePath = appendPath(target, fileName);
  std::ofstream outFile(filePath, std::ios::binary);
  if (!outFile) {
    sendError(500);
    return;
  }

  
  outFile << extractBodyfile(getBody());
  outFile.close();
  std::cout << "POST file created: " << filePath << std::endl;

  setStatusCode("200");
  setversion("HTTP/1.0");
  setStatusMessage("OK");
  buildResponse();
  // (getFd());
}

void Client::handelGET() {

  findTargetLocation();

  std::string uri = getUri();
  std::string root = _targetLocation.getRoot();
  std::string index = _targetLocation.getIndex();
  std::string locationPath = _targetLocation.getPath();
  std::string uriSuffix;
  std::string pathToAppend;

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
    sendError(404);
    return;
  }
  if (S_ISREG(statBuf.st_mode)) {
      if(_targetLocation.isMethodAllowed(GET))
        sendFile(target);
      else 
        sendError(405);
      return;

  } else if (S_ISDIR(statBuf.st_mode)) {
    if (uri[uri.length() - 1] != '/') {
      redirection(301, uri + "/");
      return;
    }
    if(!_targetLocation.isMethodAllowed(GET))
    {
        sendError(405);
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
        sendError(403);
        return;
      }
    }
  } else {
    sendError(403);
    return;
  }
}
