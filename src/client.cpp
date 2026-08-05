#include "client.hpp"
#include "../parssing/config.hpp"
#include "webserv.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include "static_utils.hpp"
#include <sys/stat.h>
#include <fstream>

Client::Client() {
}

Client::~Client() {
}

Client::Client(const Client &other) : _config(other._config), _request(other._request), _response(other._response) {
}

// Client::Client(const Config &config, const std::vector<std::string> &tokens) : _config(config), _tokens(tokens) {
// 	// set the maximum body size for the request based on the configuration
// 	_request.set_max_body_size(_config.getClientMaxBodySize());
// 	std::cout << "Client created with max body size: " << _config.getClientMaxBodySize() << std::endl;
// }

Client::Client(const Config &config, const std::vector<std::string> &tokens, int fd) : _config(config), _tokens(tokens), _fd(fd) {
	// set the maximum body size for the request based on the configuration
	_request.set_max_body_size(_config.getClientMaxBodySize());
	std::cout << "Client created with max body size: " << _config.getClientMaxBodySize() << std::endl;
}

Client &Client::operator=(const Client &other) {
	if (this != &other) {
		_config = other._config;
		_request = other._request;
		_response = other._response;
	}
	return *this;
}
#include <iostream>

void Client::receiveBuffer(int client_fd) {
	char buffer[4096];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		_request.appendData(buffer, bytesRead); 
		return ;
	} else if (bytesRead == 0) {
		std::cout << "Client disconnected on socket " << client_fd << std::endl;
		throw std::runtime_error("Client disconnected");
	} else {
		std::cerr << "Error reading from client on socket " << client_fd << std::endl;
		throw std::runtime_error("Error reading from client");
	}
}

bool Client::validateToken(std::string token) {
	// find the token in the _tokens vector
	for (std::vector<std::string>::const_iterator it = _tokens.begin(); it != _tokens.end(); ++it) {
		if (*it == token) {
			return true; // token is valid
		}
	}
	return false; // token is not valid
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
    // simple "write raw body to file at target" upload — adjust to your multipart parsing if needed
    std::ofstream out(target.c_str(), std::ios::binary);
    if (!out) {
        _response.sendError(500, "Internal Server Error");
        return;
    }
    out << _request.getBody();
    out.close();
    _response.setVersion(_request.getVersion());
    _response.setStatusCode(200, "OK");
    _response.setHeader("Content-Length", "0");
    _response.buildResponse();
}

void Client::handleStaticDELETE(const std::string &target) {
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
