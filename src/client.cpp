#include "client.hpp"
#include "../parssing/config.hpp"
#include "webserv.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include "static_utils.hpp"
#include <sys/stat.h>
#include <fstream>

Client::Client() {}

Client::~Client() {}

Client::Client(const Config &config, std::vector<std::string> *tokens, int fd)
    : _config(config), _tokens(tokens), _fd(fd) {
    _request.set_max_body_size(_config.getClientMaxBodySize());
}


Client::Client(const Client &other)
    : _config(other._config), _request(other._request), _response(other._response),
      _matchedLocation(other._matchedLocation), _tokens(other._tokens), _fd(other._fd),
      _cgiPid(other._cgiPid), _cgiOutFd(other._cgiOutFd), _cgiOutput(other._cgiOutput),
      _cgiBody(other._cgiBody), _cgiBodySent(other._cgiBodySent) {}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_config = other._config;
		_request = other._request;
		_response = other._response;
		_matchedLocation = other._matchedLocation;
		_tokens = other._tokens;
		_fd = other._fd;
		_cgiPid = other._cgiPid;
		_cgiOutFd = other._cgiOutFd;
		_cgiOutput = other._cgiOutput;
		_cgiBody = other._cgiBody;
		_cgiBodySent = other._cgiBodySent;
	}
	return *this;
}


// Getters

Request &Client::getRequest() { return _request; }
Response &Client::getResponse() { return _response; }
Location Client::getMatchedLocation() const { return _matchedLocation; }
const Config &Client::getConfig() const { return _config; }
const std::string &Client::getCgiOutput() const { return _cgiOutput; }
const std::string &Client::getCgiBody() const { return _cgiBody; }
size_t Client::getCgiBodySent() const { return _cgiBodySent; }
int Client::getFd() const { return _fd; }
pid_t Client::getCgiPid() const { return _cgiPid; }
int Client::getCgiOutFd() const { return _cgiOutFd; }


// Setters

void Client::setCgiBody(const std::string &body) {
    _cgiBody = body;
    _cgiBodySent = 0;
}
void Client::setFd(int fd) { _fd = fd; }
void Client::setCgiPid(pid_t p) { _cgiPid = p; }
void Client::setCgiOutFd(int fd) { _cgiOutFd = fd; }


// CGI methods

void Client::appendCgiOutput(const char *buf, size_t n) { _cgiOutput.append(buf, n); }
void Client::addCgiBodySent(size_t n) { _cgiBodySent += n; }


void Client::receiveBuffer(int client_fd) {
	char buffer[4096];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		_request.appendData(buffer, bytesRead); 
		return ;
	} else if (bytesRead == 0) {
		throw std::runtime_error("Client disconnected");
	} else {
		throw std::runtime_error("Error reading from client");
	}
}

void Client::sendFile(const std::string &filepath) {
    std::string content = readFile(filepath);
    if (content.empty()) {
        _response.sendError(404, "Not Found");
        return;
    }
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(200, "OK");
    _response.setHeader("Content-Type", getMimeType(filepath));
    _response.setBody(content);
    _response.buildResponse();
}

void Client::redirect(int code, const std::string &newLocation) {
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(code, "Moved Permanently");
    _response.setHeader("Location", newLocation);
    _response.setHeader("Content-Length", "0");
    _response.buildResponse();
}

void Client::processAutoIndex(const std::string &uri, const std::string &target) {
    std::string html = buildAutoIndex(target, uri);
    if (html.empty()) {
        _response.sendError(403, "Forbidden");
        return;
    }
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(200, "OK");
    _response.setHeader("Content-Type", "text/html");
    _response.setBody(html);
    _response.buildResponse();
}

void Client::handleStaticGET(const std::string &target, const std::string &uri) {
    // print message on green color
    struct stat st;
    if (stat(target.c_str(), &st) == -1) {
        _response.sendError(404, "Not Found");
        return;
    }
    if (S_ISREG(st.st_mode)) {
        sendFile(target);
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        if (uri.empty() || uri[uri.size()-1] != '/') {
            redirect(301, uri + "/");
            return;
        }
        std::string indexPath = appendPath(target, _matchedLocation.getIndex());
        struct stat ist;
        if (!_matchedLocation.getIndex().empty() && stat(indexPath.c_str(), &ist) == 0 && S_ISREG(ist.st_mode)) {
            sendFile(indexPath);
            return;
        }
        if (_matchedLocation.getAutoindex()) {
            processAutoIndex(uri, target);
            return;
        }
        _response.sendError(403, "Forbidden");
        return;
    }
    _response.sendError(403, "Forbidden");
}

void Client::handleStaticPOST(const std::string &target) {
    
    std::string filename = takenamefile(_request.getBody());
    std::ofstream out((target+ "/" + filename).c_str(), std::ios::binary);
    if (!out) {
        _response.sendError(500, "Internal Server Error");
        return;
    }
    out << takeBodyContent(_request.getBody(), getBoundary(_request.getContentType()));
    out.close();
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(200, "OK");
    _response.setHeader("Content-Length", "0");
    _response.buildResponse();
}

void Client::handleStaticDELETE(const std::string &target) {
    // print message on red color
    struct stat st;
    if (stat(target.c_str(), &st) == -1) {
        _response.sendError(404, "Not Found");
        return;
    }
    if (remove(target.c_str()) != 0) {
        _response.sendError(500, "Internal Server Error");
        return;
    }
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(200, "OK");
    _response.setHeader("Content-Length", "0");
    _response.buildResponse();
}


void Client::processStatic() {
    const std::string &uri = _request.getUri();
    if (!isPathSafe(uri)) {
        _response.sendError(403, "Forbidden");
        return;
    }

    std::string root = _matchedLocation.getRoot();
    std::string locPath = _matchedLocation.getPath();
    std::string suffix ;
	if (uri.compare(0, locPath.length(), locPath) == 0) {
		suffix = uri.substr(locPath.length());
	} else {
		suffix = uri;
	}
	
    std::string target = appendPath(root, suffix);

    if (_request.getMethod() == "GET")
        handleStaticGET(target, uri);
    else if (_request.getMethod() == "POST")
        handleStaticPOST(target);
    else if (_request.getMethod() == "DELETE")
        handleStaticDELETE(target);
    else
        _response.sendError(501, "Not Implemented");
}

void Client::matchLocation()
{
    const std::vector<Location> &locations = _config.getLocations();
    const std::string &requestUri = _request.getUri();

    const Location *bestMatch = NULL;
    size_t bestLength = 0;

    for (std::vector<Location>::const_iterator it = locations.begin();
        it != locations.end(); ++it)
    {
        const std::string &locationPath = it->getPath();

        if (requestUri.compare(0, locationPath.length(), locationPath) != 0)
            continue;

        bool valid = false;
        if (requestUri.length() == locationPath.length())
            valid = true;
        else if (locationPath == "/")
            valid = true;
        else if (requestUri[locationPath.length()] == '/')
            valid = true;

        if (!valid)
            continue;

        if (locationPath.length() > bestLength)
        {
            bestMatch = &(*it);
            bestLength = locationPath.length();
        }
    }

    if (bestMatch)
        _matchedLocation = *bestMatch;

    const std::string &locPath = _matchedLocation.getPath();
    if (locPath == "/signup" || locPath == "/login" || locPath == "/cgi")
    {
        std::string root = _matchedLocation.getRoot();
        std::string scriptPath;

        if (locPath == "/signup")
            scriptPath = appendPath(root, "/signup.py");
        else if (locPath == "/login")
            scriptPath = appendPath(root, "/login.py");
        else 
        {
            scriptPath = appendPath(root, "/run_c.py");
            // std::cout << "\033[33mCGI script path: " << scriptPath << "\033[0m" << std::endl;
        }
        _matchedLocation.setTargetPath(scriptPath);
    }
}

bool Client::isMethodeAllowed() {
    const std::vector<std::string> &allowedMethods = _matchedLocation.getMethods();
    if (allowedMethods.empty()) {
        return true;
    }
    const std::string &requestMethod = _request.getMethod();
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
        if (*it == requestMethod) {
            return true;
        }
    }
    return false;
}

void Client::checkAccess() {
    const std::string &locationPath = _matchedLocation.getPath();
    const std::string &uri = _request.getUri();
    bool valid = validateToken(_request.getToken());

    if (locationPath == "/static") {
        return;
    }

    if (uri == "/login.html" || uri == "/signup.html" || locationPath == "/login" || locationPath == "/signup") {
        if (valid)
            throw redirectException("/");
        return;
    }

    if (!valid)
        throw redirectException("/login.html");
}

bool Client::validateToken(std::string token) {
    if (token.empty() || !_tokens) return false;
    for (std::vector<std::string>::const_iterator it = _tokens->begin(); it != _tokens->end(); ++it)
    {
        if (*it == token) 
            return true;
    }
    return false;
}
