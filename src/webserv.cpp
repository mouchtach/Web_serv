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
    _clients[fd] = Client(config, &tokens, fd);
}

FD_type WebServ::getFDType(int fd) {
	if (_fdInfos.find(fd) != _fdInfos.end()) {
		return _fdInfos[fd].type;
	}
	throw std::runtime_error("FD not found");
}

void WebServ::closeFd(int fd) {
	close(fd);
	_fdInfos.erase(fd);
	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
		if (it->fd == fd) {
			_pollfds.erase(it);
			break;
		}
	}
}

void WebServ::changePollToWrite(int fd) {
	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
		if (it->fd == fd) {
			it->events = POLLOUT;
			break;
		}
	}
}

void WebServ::removeCgiFd(int fd) {
    closeFd(fd);
}

void WebServ::removeClient(int fd) {
	// use closeFd() to close the fd and remove it from _fdInfos and _pollfds
	closeFd(fd);
	_clients.erase(fd);
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

	FD_type type = getFDType(fd);   // <-- check first, don't just index _clients blindly

	if (type == CGI_IN) {
		cgiWriteBody(fd);
		return;
	}
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
	} else if (type == CGI_OUT) {
		cgiReadOutput(fd);
	} else {
		std::cerr << "Unknown FD type for socket " << fd << std::endl;
	}
}

void WebServ::cgiWriteBody(int fd) {
    Client *client = static_cast<Client*>(_fdInfos[fd].obj);
    const std::string &body = client->getCgiBody();
    size_t sent = client->getCgiBodySent();

    if (sent >= body.size()) {
        closeFd(fd);
        return;
    }
    ssize_t n = write(fd, body.c_str() + sent, body.size() - sent);
    if (n < 0) {
        closeFd(fd);
        return;
    }
    client->addCgiBodySent(n);
}

void WebServ::cgiReadOutput(int fd) {
    Client *client = static_cast<Client*>(_fdInfos[fd].obj);
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n > 0) {
        client->appendCgiOutput(buf, n);
        return;
    }
    closeFd(fd);   // n == 0 (EOF) or n < 0 (error) — CGI is done sending either way
    int status;
    waitpid(client->getCgiPid(), &status, 0);
    finalizeCgiResponse(*client);
    changePollToWrite(client->getFd());
}

// webserv.cpp

void WebServ::parseCgiOutput(const std::string &raw) {
    _result.statusCode = 200;
    _result.statusMsg = "OK";
    _result.headers.clear();   // must clear — map from the PREVIOUS request would leak in otherwise
    _result.body.clear();
    size_t pos = 0;

    while (pos < raw.size()) {
        size_t lineEnd = raw.find('\n', pos);
        if (lineEnd == std::string::npos) break;
        std::string line = raw.substr(pos, lineEnd - pos);
        pos = lineEnd + 1;
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        size_t s = value.find_first_not_of(" \t");
        value = (s == std::string::npos) ? "" : value.substr(s);

        if (key == "Status") {
            _result.statusCode = std::atoi(value.c_str());
            size_t sp = value.find(' ');
            _result.statusMsg = (sp != std::string::npos) ? value.substr(sp + 1) : "OK";
        } else {
            _result.headers[key] = value;
        }
    }
    _result.body = raw.substr(pos);
}

void WebServ::storeCgiToken(Client &client) {
    if (_result.statusCode != 200 || client.getMatchedLocation().getPath() != "/login")
        return;
    std::map<std::string, std::string>::const_iterator it = _result.headers.find("X-Auth-Token");
    if (it != _result.headers.end())
        _tokens.push_back(it->second);
}

void WebServ::buildCgiResponse(Client &client) {
    Response &response = client.getResponse();
    response.setVersion(client.getRequest().getVersion());
    response.setStatusCode(_result.statusCode, _result.statusMsg);
    for (std::map<std::string, std::string>::const_iterator it = _result.headers.begin(); it != _result.headers.end(); ++it)
        response.setHeader(it->first, it->second);
    if (_result.headers.find("Content-Type") == _result.headers.end())
        response.setHeader("Content-Type", "text/html");
    response.setBody(_result.body);
    response.buildResponse();
}

void WebServ::finalizeCgiResponse(Client &client) {
    parseCgiOutput(client.getCgiOutput());
    storeCgiToken(client);
    buildCgiResponse(client);
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
	// request.displayRequest();

    handleRequest(fd);
	// changePollToWrite(fd);
}

#include <sys/stat.h>

std::vector<std::string> WebServ::buildCgiEnv(Client &client, const std::string &scriptPath) {
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + client.getRequest().getMethod());
    env.push_back("CONTENT_LENGTH=" + intToStr((int)client.getRequest().getBody().size()));
    env.push_back("CONTENT_TYPE=" + client.getRequest().getContentType());
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SERVER_PROTOCOL=" + client.getRequest().getVersion());
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("HTTP_COOKIE=" + client.getRequest().getToken());

    return env;
}

// #include <sys/wait.h>
// #include <unistd.h>
// #include <fcntl.h>
#include <sys/stat.h>



void WebServ::startCgi(int client_fd) {
    Client &client = _clients[client_fd];
    const Location &loc = client.getMatchedLocation();

    std::string scriptPath = loc.getTargetPath();
    std::string interpreter = loc.getCgiPath();

    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0) {
        client.getResponse().sendError(404, "Not Found");
        changePollToWrite(client_fd);
        return;
    }

    int inPipe[2];
    int outPipe[2];
    if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
        client.getResponse().sendError(500, "Internal Server Error");
        changePollToWrite(client_fd);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        client.getResponse().sendError(500, "Internal Server Error");
        changePollToWrite(client_fd);
        return;
    }

    if (pid == 0) {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        std::vector<std::string> envStrs = buildCgiEnv(client, scriptPath);
        std::vector<char*> envp;
        for (size_t i = 0; i < envStrs.size(); ++i)
            envp.push_back(const_cast<char*>(envStrs[i].c_str()));
        envp.push_back(NULL);

        char *argv[] = {
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        execve(interpreter.c_str(), argv, envp.data());
        std::exit(1);
    }

    close(inPipe[0]);
    close(outPipe[1]);
    fcntl(inPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(outPipe[0], F_SETFL, O_NONBLOCK);

    client.setCgiPid(pid);
    client.setCgiOutFd(outPipe[0]);
    client.setCgiBody(client.getRequest().getBody());
    client.setFd(client_fd);

    addinfo(outPipe[0], CGI_OUT, &client);
    addpollfd(outPipe[0], POLLIN);

    if (client.getCgiBody().empty()) {
        close(inPipe[1]);
    } else {
        addinfo(inPipe[1], CGI_IN, &client);
        addpollfd(inPipe[1], POLLOUT);
    }
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
        startCgi(fd);
        return;       
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
	