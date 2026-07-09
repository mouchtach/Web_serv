#include "client.hpp"
#include "utils.h"
#include <sys/stat.h>
#include <unistd.h>

Client::Client(): _fd(-1), _config(), _targetLocation() , _request(), _response() {}

Client::Client(int fd, const Config &config) : _fd(fd), _cgiPid(-1),  _config(config), _cgi_inputfd(-1), _cgi_outputfd(-1) {
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

bool endsWith(const std::string& str, const std::string& suffix) {
    std::cout << "Checking if '" << str << "' ends with '" << suffix << "'" << std::endl;
    if (str.length() < suffix.length()) 
    {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

void Client::processResponse() {
  findTargetLocation();
  std::string root = _targetLocation.getRoot();
  std::string locationPath = _targetLocation.getPath();
  std::string pathToAppend;
  std::string uri = _request.getUri();

  if (isPathsafe(uri) == false)
  {
      _response.sendError(403);
      return;
  }
  if (_targetLocation.isMethodAllowed(_request.getMethod()) == false)
  {
      _response.sendError(405);
      return;
  }
  if (_targetLocation.hasredirection()) {
      redirection(_targetLocation.getReturn().first, _targetLocation.getReturn().second);
      return;
  }
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
  std::cout << "CHECK "  << _targetLocation.getCgiExtension() << std::endl;
  if (!_targetLocation.getCgiExtension().empty() && endsWith(target, _targetLocation.getCgiExtension())) {
      handleCGI(target);
      return;
  }
  if (_request.getMethod() == GET) {
      handelGET(target,  uri);
  } else if (_request.getMethod() == POST) {
      handelPOST(target);
  } else if (_request.getMethod() == DELETE) {
      handelDELETE(target);
  }
}

void Client::handleCGI(std::string& target) {

    std::cout << "Handling CGI for target: " << target << std::endl;
    int in[2], out[2];
    pipe(in);
    pipe(out);

    _cgiPid = fork();
    if (_cgiPid == 0) {
        // child
        close(in[1]);
        close(out[0]);
        dup2(in[0], STDIN_FILENO);
        dup2(out[1], STDOUT_FILENO);
        close(in[0]);
        close(out[1]);

        std::string test = "hello from CGI child\n";
        write(STDOUT_FILENO, test.c_str(), test.size());
        exit(0);
    }

    // parent
    close(in[0]);
    close(out[1]);
    close(in[1]); // nothing to send to stdin for this test

    _cgi_inputfd  = out[0]; // fd we READ cgi output from
    _cgi_outputfd = -1;

    pollfd pfd;
    pfd.fd = out[0];
    pfd.events = POLLIN;   // fixed: watch for readable data
    _pollfds->push_back(pfd);
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

void Client::handelDELETE(std::string& target) {
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

void Client::handelPOST(std::string target){
  struct stat statBuf;
  if (stat(target.c_str(), &statBuf) == -1)
    return(_response.sendError(404));
  std::string fileName = extractFileName(_request.getBody());
  std::string filePath = appendPath(target, fileName);
  std::ofstream outFile(filePath, std::ios::binary);
  if (!outFile) 
    return(_response.sendError(500));
  outFile << extractBodyfile(_request.getBody());
  outFile.close();
  std::cout << "POST file created: " << filePath << std::endl;
  _response.setStatusCode("200");
  _response.setversion("HTTP/1.0");
  _response.setStatusMessage("OK");
  _response.buildResponse();
}

void Client::handelGET(std::string target, std::string uri) {

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
    std::string indexPath = target + "/" + _targetLocation.getIndex();
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
