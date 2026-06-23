#include "Webserv.hpp"
#include "config/ParssingConf.hpp"
#include "server/client.hpp"
#include "server/server.hpp"
// #include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>

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
  parser.parseConfig(configFile);
  const std::vector<Config> &configs = parser.getConfigs();

  for (size_t i = 0; i < configs.size(); ++i) {
    Server server(configs[i]);
    _servers.push_back(server);
    pollfd pfd;
    pfd.fd = server.getFd();
    pfd.events = POLLIN;
    _pollfds.push_back(pfd);
  }
}

void Webserv::newConnection(int serverFd) {
  int clientFd = accept(serverFd, NULL, NULL);
  if (clientFd < 0)
    throw std::runtime_error("Failed to accept new connection");
  Client client(clientFd, getServerByFd(serverFd)->getConfig());
  _clients.push_back(client);
  pollfd pfd;
  pfd.fd = clientFd;
  pfd.events = POLLIN;
  _pollfds.push_back(pfd);
  std::cout << "New connection accepted: client fd " << clientFd
            << " on server fd " << serverFd << std::endl;
}

void Webserv::readFromClient(int clientFd) {
  Client *client = getClientByFd(clientFd);
  if (!client)
    throw std::runtime_error("Client not found");
  char buffer[4096];
  ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
  if (bytesRead < 0)
    throw std::runtime_error("Failed to read from client");
  else if (bytesRead == 0) {
    if (client->Request::isheaderComplete() &&
        !client->Request::isRequestComplete())
      std::cerr << "Client " << client->getFd()
                << " closed with incomplete body (Content-Length mismatch)"
                << std::endl;
    removeClient(clientFd);
    return;
  }
  client->Request::appendrequest(std::string(buffer, bytesRead));
  if (client->Request::isheaderComplete()) {
    if (client->Request::isRequestComplete()) {
        std::cout << "Request complete for client fd " << clientFd << std::endl;
      client->processResponse();
      readyToSend(clientFd);
      client->clear_rawRequest();
    } else {
      // Headers complete but body incomplete - wait for more data or client to
      // close
      std::cout << "Headers complete but body not fully received for client fd "
                << clientFd << std::endl;
      return;
    }
  }
}

void Webserv::Start() {

  while (true) {
    int ret = poll(_pollfds.data(), _pollfds.size(), -1); // Infinite wait
    if (ret < 0) {
      throw std::runtime_error("Poll failed");
    }
    for (size_t i = 0; i < _pollfds.size(); ++i) {
      if (_pollfds[i].revents & POLLIN) {
        if (is_server(_pollfds[i].fd))
          newConnection(_pollfds[i].fd);
        else
          readFromClient(_pollfds[i].fd);
      } else if (_pollfds[i].revents & POLLOUT) {
        Client *client = getClientByFd(_pollfds[i].fd);
        if (client) {
          const std::string &response = client->getRawResponse();
          std::cout << "response to send: " << response << std::endl;
          std::cout << "Sending response to client fd " << client->getFd()
                    << ": " << response << std::endl;
          ssize_t bytesSent =
              send(client->getFd(), response.c_str(), response.size(), 0);
          if (bytesSent < 0) {
            throw std::runtime_error("Failed to send response to client");
          }
          removeClient(client->getFd());
        }
      } else if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        std::cout << "Error event on fd " << _pollfds[i].fd << std::endl;
        // Handle error event
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
  for (size_t i = 0; i < _clients.size(); ++i) {
    if (_clients[i].getFd() == fd) {
      return &_clients[i];
    }
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
  for (std::vector<Client>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->getFd() == clientFd) {
      _clients.erase(it);
      break;
    }
  }
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
    std::cout << "Ready to send response to client fd " << clientFd
              << std::endl;
    pfd->events = POLLOUT;
  }
}

// void Webserv::PrintServers() const {
//   std::cout << "Configured Servers:" << std::endl;
//   for (size_t i = 0; i < _servers.size(); ++i) {
//     const Config &cfg = _servers[i].getConfig();
//     std::cout << "Server " << i + 1 << ":" << std::endl;
//     std::cout << "  Port: " << cfg.getPort() << std::endl;
//     std::cout << "  Server Name: " << cfg.getServerName() << std::endl;
//     std::cout << "  Root: " << cfg.getRoot() << std::endl;
//     std::cout << "  Index: " << cfg.getIndex() << std::endl;
//     std::cout << "  Autoindex: " << (cfg.getAutoindex() ? "on" : "off")
//               << std::endl;
//     std::cout << "  Client Max Body Size: " << cfg.getClientMaxBodySize()
//               << std::endl;

//     const std::map<int, std::string> &errorPages = cfg.getErrorPages();
//     if (!errorPages.empty()) {
//       std::cout << "  Error Pages:" << std::endl;
//       for (std::map<int, std::string>::const_iterator it = errorPages.begin();
//            it != errorPages.end(); ++it) {
//         std::cout << "    Code: " << it->first << ", Path: " << it->second
//                   << std::endl;
//       }
//     }

//     const std::vector<LocationConfig *> &locations = cfg.getLocations();
//     if (!locations.empty()) {
//       std::cout << "  Locations:" << std::endl;
//       for (size_t j = 0; j < locations.size(); ++j) {
//         const LocationConfig *loc = locations[j];
//         std::cout << "    Location Path: " << loc->getPath() << std::endl;
//         // Print other location-specific configurations as needed
//       }
//     }
//   }
// }