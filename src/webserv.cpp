#include "webserv.hpp"
#include "../parssing/configparssing.hpp"
#include "../parssing/utils.hpp"
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
	_clients[fd] = Client(config, tokens, fd);
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
		// std::cout << "New connection accepted on socket " << client_fd << std::endl;
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
	// non blokiing send
	Client &client = _clients[fd];
	Response &response = client.getResponse();
	const std::string &responseStr = response.getRawResponse();
	size_t sentBytes = response.getSentBytes();
	// std::cout << "bytes sent so far: " << sentBytes << std::endl;
	if (sentBytes >= responseStr.size()) {
		// std::cout << "Full response sent to client on socket " << fd << std::endl;
		// std::cout << "Response : " << responseStr << std::endl;
		removeClient(fd);
		// exit(0);
		return;
	}
	ssize_t bytesSent = send(fd, responseStr.c_str() + sentBytes, responseStr.size() - sentBytes, 0);
	if (bytesSent < 0) {
		std::cerr << "Error sending response to client on socket " << fd << std::endl;
		removeClient(fd);
		return;	
	}
	response.addBytesSent(bytesSent);
	if (!response.isFullySent()) {
		std::cout << "Partial response sent to client on socket " << fd << std::endl;
	} else {
		// std::cout << "Full response sent to client on socket " << fd << std::endl;
		// std::cout << "Response : " << responseStr << std::endl;
		removeClient(fd);

		// exit(0);
	}
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
	// print the request for debugging
	request.displayRequest();
	// std::cout << "Request received from client on socket " << fd << ":\n" << request.getBuffer() << std::endl;
    handleRequest(fd);

	changePollToWrite(fd);
}


std::vector<std::string> WebServ::buildCgiEnv(Client &client, const std::string &scriptPath) {
    std::vector<std::string> env;
    Request &req = client.getRequest();
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("SCRIPT_NAME=" + scriptPath);
    env.push_back("SERVER_PROTOCOL=" + req.getVersion());
    std::ostringstream cl;
    cl << req.getBody().size();
    env.push_back("CONTENT_LENGTH=" + cl.str());

    std::map<std::string,std::string>::const_iterator it = req.getHeaders().find("content-type");
    if (it != req.getHeaders().end())
        env.push_back("CONTENT_TYPE=" + it->second);
    return env;
}

void WebServ::startCgi(int client_fd) {

}




void WebServ::handleRequest(int fd) {
    Client &client = _clients[fd];
    client.matchLocation();

	try {
    	client.checkAccess();
	}
	catch (const redirectException &e) {
		client.redirect(302, e.getRedirectUrl());
		changePollToWrite(fd);
		return;
	}
    if (!client.isMethodeAllowed())
        throw HttpException(405, "Method Not Allowed");

    std::string path = client.getMatchedLocation().getPath();
    if (path == "/signup" || path == "/login" || path == "/cgi") {
		// print message on red color
		std::cout << "\033[31mStarting CGI process for path: " << path << "\033[0m" << std::endl;
        startCgi(fd);   // NOT cgiProcess — that's for the poll loop
        return;         // don't changePollToWrite yet; wait for CGI to finish
    }
	
    client.processStatic();
    changePollToWrite(fd);

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

void WebServ::finalizeCgiResponse(Client &client) {
    const std::string &out = client.getCgiOutput();
    size_t headerEnd = out.find("\r\n\r\n");
    if (headerEnd == std::string::npos) headerEnd = out.find("\n\n");

    std::string headerPart ;
	if (headerEnd != std::string::npos) 
		headerPart = out.substr(0, headerEnd);
	else 
		headerPart = "";
    std::string body ;
	if (headerEnd != std::string::npos) 
		body = out.substr(headerEnd + (out[headerEnd+2]=='\r'?4:2));
	else 
		body = out;

    int statusCode = 200;
    std::string statusMsg = "OK";
    std::string contentType = "text/html";
    std::string authToken;

    std::istringstream hs(headerPart);
    std::string line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        while (!val.empty() && val[0] == ' ') val.erase(0,1);

        if (key == "Status") {
            statusCode = atoi(val.c_str());
            size_t sp = val.find(' ');
            statusMsg = (sp != std::string::npos) ? val.substr(sp+1) : "OK";
        } else if (key == "Content-Type") {
            contentType = val;
        } else if (key == "X-Auth-Token") {
            authToken = val;
        }
    }

    if (!authToken.empty())
        _tokens.push_back(authToken); // now validateToken() will accept it

    Response &resp = client.getResponse();
    resp.setStatusCode(statusCode, statusMsg);
    resp.setHeader("Content-Type", contentType);
    resp.setHeader("Content-Length", intToStr(body.size()));
    if (!authToken.empty())
        resp.setHeader("Set-Cookie", "token=" + authToken); // send it to the browser too
    resp.setBody(body);
    resp.buildResponse();
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
	