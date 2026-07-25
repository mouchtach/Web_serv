#include "client.hpp"
#include "../parssing/config.hpp"
#include "webserv.hpp"

#include <sys/socket.h>
#include <unistd.h>

Client::Client() {
}

Client::~Client() {
}

Client::Client(const Client &other) : _config(other._config), _request(other._request), _response(other._response) {
}

Client::Client(const Config &config, const std::vector<std::string> &tokens) : _config(config), _tokens(tokens) {
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
