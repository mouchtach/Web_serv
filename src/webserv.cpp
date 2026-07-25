#include "webserv.hpp"
#include "../parssing/configparssing.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
WebServ::WebServ() {
}

WebServ::~WebServ() {
}

WebServ::WebServ(const WebServ &other) : _configs(other._configs) {
}

WebServ &WebServ::operator=(const WebServ &other) {
	if (this != &other)
		_configs = other._configs;
	return *this;
}


void WebServ::addinfo(int fd, FD_type type, void *obj) {
	FD_info info;
	info.fd = fd;
	info.type = type;
	info.obj = obj;
	_fdInfos[fd] = info;
}

void WebServ::addpollfd(int fd, short events) {
	pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollfds.push_back(pfd);
}

void WebServ::addserver(int fd, const Config &config) {
	_servers[fd] = Server(config);
}

void WebServ::addclient(int fd, const Config &config, std::vector<std::string> &tokens) {
	_clients[fd] = Client(config, tokens);
}

FD_type WebServ::getFDType(int fd) {
	if (_fdInfos.find(fd) != _fdInfos.end()) {
		return _fdInfos[fd].type;
	}
	throw std::runtime_error("FD not found");
}


void WebServ::newConnection(int server_fd) {
	try {
		int client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0) {
			throw std::runtime_error("Failed to accept new connection");
		}
		if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
			throw std::runtime_error("Failed to set client socket to non-blocking");
		}
		addclient(client_fd, _servers[server_fd].getConfig(), _tokens);  // Create a Client object for this socket
		addpollfd(client_fd, POLLIN); // Add the client socket to the pollfd vector
		addinfo(client_fd, FD_CLIENT, &_clients[client_fd]);  // Store the client socket info
		// set_max_body_size(_servers[server_fd].getConfig().getClientMaxBodySize());
		std::cout << "New connection accepted on socket " << client_fd << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Error in newConnection: " << e.what() << std::endl;
	}
}

#include <iostream>

void WebServ::setup() {
	// setup server socket bind lestin fcntl setsockopt
	for (size_t i = 0; i < _configs.size(); ++i) {

		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(_configs[i].getPort());

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			throw std::runtime_error("Failed to create socket");
		}
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
			throw std::runtime_error("Failed to set socket to non-blocking");
		}
		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)) < 0) {
			throw std::runtime_error("Failed to set socket options");
		}
		if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
			throw std::runtime_error("Failed to bind socket");
		}
		// Use system maximum backlog if available
		if (listen(fd, SOMAXCONN) < 0) {
			throw std::runtime_error("Failed to listen on socket");
		}
		addserver(fd, _configs[i]);  // Create a Server object for this socket
		addpollfd(fd, POLLIN); // Add the server socket to the pollfd vector
		addinfo(fd, FD_SERVER, &_servers[fd]);  // Store the server socket info
		std::cout  << "Server listening on port " << _configs[i].getPort() << std::endl;
	}
}

void WebServ::polloutprocess(int fd) {
	// Handle POLLOUT events for the given file descriptor
	// This is where you would write data to the client or CGI process
	// For now, we'll just print a message
	std::cout << "Ready to write to socket " << fd << std::endl;
}

void WebServ::pollinprocess(int fd) {

	FD_type type = getFDType(fd);
	if (type == FD_SERVER) {
		newConnection(fd);
	} else if (type == FD_CLIENT) {
		readFromClient(fd);
	} else if (type == CGI) {
		cgiProcess(fd);
	} else {
		std::cerr << "Unknown FD type for socket " << fd << std::endl;
	}
}

void WebServ::readFromClient(int fd)
{
    Client &client = _clients[fd];
    Request &request = client.getRequest();
    char buffer[4096];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
    if (n == 0)
    {
		std::cout << "Client disconnected on socket " << fd << std::endl;
        removeClient(fd);
        return;
    }
    if (n < 0)
    {
		std::cout << "Error reading from client on socket " << fd << std::endl;
        removeClient(fd);
        return;
    }
    request.appendData(buffer, n);
    request.parse();
    if (!request.isRequestComplete())
        return;

    // Process request
    handleRequest(fd);

    // Ready to send
    changePollToWrite(fd);
}

void WebServ::set_CgiRequirements(Client &client) {
	int fds[2];
	if (pipe(fds) == -1) {
		throw std::runtime_error("Failed to create pipe for CGI");
	}
	addpollfd(fds[0], POLLIN);
	addinfo(fds[0], CGI, &client);
	addpollfd(fds[1], POLLOUT);
	addinfo(fds[1], CGI, &client);
}


void WebServ::cgiProcess(int fd) {

	set_CgiRequirements(_clients[fd]);
}


void WebServ::handleRequest(int fd) {
	Client &client = _clients[fd];

	client.matchLocation();
	client.checkAccess();
	if (!client.isMethodeAllowed()) {
		throw HttpException(405, "Method Not Allowed");
	}
	if (client.getMatchedLocation().getPath() == "/sigup" || client.getMatchedLocation().getPath() == "/login" || client.getMatchedLocation().getPath() == "/cgi") {
		cgiProcess(fd);
	} 



}

void WebServ::start() {

	while (true) {
		int ret = poll(_pollfds.data(), _pollfds.size(), -1);
		if (ret < 0) {
			throw std::runtime_error("Poll failed");
		}
		for (size_t i = 0; i < _pollfds.size(); ++i) {
			try {
				if (_pollfds[i].revents & POLLIN) 
					pollinprocess(_pollfds[i].fd);
				else if (_pollfds[i].revents & POLLOUT) 
					polloutprocess(_pollfds[i].fd);
				else if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
					std::cerr << "Error on socket " << _pollfds[i].fd << std::endl;
					removeClient(_pollfds[i].fd);
				}
			} catch (const std::exception &e) {
				std::cerr << "Exception in poll processing for socket " << _pollfds[i].fd << ": " << e.what() << std::endl;
				removeClient(_pollfds[i].fd);
			}  
			
		}
	}
}

// #include <iostream>
void WebServ::parsing(const std::string &filename) {
	ConfigParssing configParser(filename);
	configParser.ReadConfig();
	configParser.removeComments();
	configParser.tokenize();
	configParser.parseConfig();
	configParser.validate();
	_configs = configParser.getConfigs();
}
	