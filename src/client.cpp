#include "client.hpp"
#include "../parssing/config.hpp"
#include "webserv.hpp"

#include <sys/socket.h>
#include <unistd.h>

Client::Client() {
}

Client::~Client() {
}

Client::Client(const Client &other) : _config(other._config) {
}

Client::Client(const Config &config) : _config(config) {
}

Client &Client::operator=(const Client &other) {
	if (this != &other)
		_config = other._config;
	return *this;
}
#include <iostream>

ssize_t Client::receivebuffer(int client_fd) {
	char buffer[10];
	ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		// buffer[bytesRead] = '\0'; // Null-terminate the buffer
		// std::cout << buffer ;
		_request.appendData(buffer, bytesRead); 
		return bytesRead;
	} else if (bytesRead == 0) {
		std::cout << "Client disconnected on socket " << client_fd << std::endl;
		return 0; // Connection closed
	}
	std::cerr << "Error reading from client on socket " << client_fd << std::endl;
	return -1; // Error occurred
}
