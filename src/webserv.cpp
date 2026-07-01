#include "Webserv.hpp"
#include "../http/httpexception.hpp"
#include "../config/ParssingConf.hpp"
#include "../server/client.hpp"
#include "../server/server.hpp"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cerrno> 
#include <cstring>

bool Webserv::is_server(int fd) const {
  for (size_t i = 0; i < _servers.size(); ++i) {
    if (_servers[i].getFd() == fd) {
      return true;
    }
  }
  return false;
}

void Webserv::setupServers(const std::string &configFile) {

    ParssingConf parser;
    try {
        std::cout << "\033[32mParsing config file: " << configFile << "\033[0m" << std::endl;
        parser.parseConfig(configFile);
    } catch (const std::exception &e) {
        std::cerr << "\033[31mError parsing config file: " << e.what() << "\033[0m" << std::endl;
        throw;
    }
    std::cout << "\033[32mSetting up servers...\033[0m" << std::endl;
    const std::vector<Config> &configs = parser.getConfigs();
    for (size_t i = 0; i < configs.size(); ++i) {
        try {
            Server server(configs[i]);
            _servers.push_back(server);
            pollfd pfd;
            pfd.fd = server.getFd();
            pfd.events = POLLIN;
            _pollfds.push_back(pfd);
        } catch (const std::exception &e) {
            std::cerr << "\033[31mError setting up server on port " << configs[i].getPort() << ": " << e.what() << "\033[0m" << std::endl;
            throw;  
        }
    }
}

void Webserv::newConnection(int serverFd) {
  int clientFd = accept(serverFd, NULL, NULL);
  if (clientFd < 0)
    throw std::runtime_error("Failed to accept new connection");
  if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    throw std::runtime_error("Failed to set client socket to non-blocking");
  Client client(clientFd, getServerByFd(serverFd)->getConfig());
  _clients.push_back(client);
  _clientMap[clientFd] = client;
  pollfd pfd;
  pfd.fd = clientFd;
  pfd.events = POLLIN;
  _pollfds.push_back(pfd);
  std::cout << "\033[32mNew connection accepted: client fd " << clientFd << " on server fd " << serverFd << "\033[0m" << std::endl;
}

void Webserv::readFromClient(int clientFd) {
  Client *client = getClientByFd(clientFd);
  try {
    if (!client)
      throw std::runtime_error("Client not found");
    char buffer[4096];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
    // std::cout << "buffer: " << std::string(buffer, bytesRead) << std::endl;
    if (bytesRead < 0)
      throw std::runtime_error("Failed to read from client");
    else if (bytesRead == 0) {
      if (client->_request.isheaderComplete() && !client->_request.isRequestComplete())
        throw std::runtime_error("Client closed connection before sending complete request body");
      removeClient(clientFd);
      return;
    }
    client->_request.appendrequest(std::string(buffer, bytesRead));
    if (client->_request.isheaderComplete()) 
    {
      if (client->_request.getMethod() == POST && client->_request.getContentLength() > client->getConfig().getClientMaxBodySize()) 
      {
        client->_response.sendError(413);
        readyToSend(clientFd);
        std::cerr << "\033[31mRequest body too large from client fd " << clientFd << "\033[0m" << std::endl;
        return;
      }
      if (client->_request.isRequestComplete()) 
      {
        client->processResponse();
        readyToSend(clientFd);
        client->_request.clear_rawRequest();
      }
    }
    } catch (const HttpException &e) {
        client->_response.sendError(e.getStatusCode());
        readyToSend(clientFd);
        std::cerr << "\033[31mBad request from client fd " << clientFd << ": " << e.getMessage() << "\033[0m" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "\033[31mError reading from client fd " << clientFd << ": " << e.what() << "\033[0m" << std::endl;
        removeClient(clientFd);
    }
}

void Webserv::Start() {
    while (true) {
        int ret = poll(_pollfds.data(), _pollfds.size(), -1); 
        if (ret < 0) {
            throw std::runtime_error("Poll failed");
        }
        for (size_t i = 0; i < _pollfds.size(); ++i) 
        {
            if (_pollfds[i].revents & POLLIN) 
            {
                if (is_server(_pollfds[i].fd))
                    newConnection(_pollfds[i].fd);
                else
                    readFromClient(_pollfds[i].fd);
            } 
            else if (_pollfds[i].revents & POLLOUT) 
            {
                try {
                    Client *client = getClientByFd(_pollfds[i].fd);
                    if (!client) {
                        removeClient(_pollfds[i].fd);
                        continue;
                    }
                    const std::string &response = client->_response.getRawResponse();
                    size_t offset = client->_response.getSentBytes();
                    size_t remaining = response.size() - offset;
                    ssize_t bytesSent = send(_pollfds[i].fd, response.c_str() + offset, remaining, 0);
                    if (bytesSent < 0) {
                        continue;
                    }
                    if (bytesSent == 0) {
                        removeClient(_pollfds[i].fd);
                        continue;
                    }
                    client->_response.setSentBytes(offset + bytesSent);
                    if (client->_response.getSentBytes() >= response.size()) {
                        std::cout << "\033[32mFull response sent to fd " << _pollfds[i].fd << "\033[0m" << std::endl;
                        removeClient(_pollfds[i].fd);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "\033[31mError sending response: " << e.what() << "\033[0m" << std::endl;
                    removeClient(_pollfds[i].fd);
                }
            } else if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // std::cerr << "\033[31mError on fd " << _pollfds[i].fd << ", closing connection\033[0m" << std::endl;
                removeClient(_pollfds[i].fd);
            }
        }
    }
}

Server *Webserv::getServerByFd(int fd) {
  for (size_t i = 0; i < _servers.size(); ++i) {
    if (_servers[i].getFd() == fd) {
      return &_servers[i];
    }
  }
  return nullptr;
}

Client *Webserv::getClientByFd(int fd) {
  std::map<int, Client>::iterator it = _clientMap.find(fd);
  if (it != _clientMap.end()) {
    return &(it->second);
  }
  return nullptr;
}

pollfd *Webserv::getPollfdByFd(int fd) {
  for (size_t i = 0; i < _pollfds.size(); ++i) {
    if (_pollfds[i].fd == fd) {
      return &_pollfds[i];
    }
  }
  return nullptr;
}

void Webserv::removeClient(int clientFd) {
  _clientMap.erase(clientFd);
  for (std::vector<pollfd>::iterator it = _pollfds.begin();
       it != _pollfds.end(); ++it) {
    if (it->fd == clientFd) {
      _pollfds.erase(it);
      break;
    }
  }
  close(clientFd);
}

void Webserv::readyToSend(int clientFd) {
  pollfd *pfd = getPollfdByFd(clientFd);
  if (pfd) {
    pfd->events = POLLOUT;
  }
}