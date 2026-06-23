#include "client.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

void Client::processResponse() {
  if (Request::getMethod() == GET) {
    handelGET();
    return;
  } else if (Request::getMethod() == POST) {
    handelPOST();
  } else if (Request::getMethod() == DELETE) {
    handelDELETE();
  }
}

#include <ctime>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>

// ─── Helper: format file size ────────────────────────────────────────────────

static std::string formatSize(off_t size) {
  std::ostringstream ss;
  if (size < 1024)
    ss << size << " B";
  else if (size < 1024 * 1024)
    ss << size / 1024 << " KB";
  else
    ss << size / (1024 * 1024) << " MB";
  return ss.str();
}

// ─── Helper: format timestamp ────────────────────────────────────────────────

static std::string formatTime(time_t t) {
  char buf[32];
  struct tm *tm = localtime(&t);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
  return std::string(buf);
}

// ─── Core: build HTML directory listing ──────────────────────────────────────

std::string Client::buildAutoIndex(const std::string &dirPath, const std::string &uri) {
  DIR *dir = opendir(dirPath.c_str());
  if (!dir)
    return "";

  struct dirent *entry;
  std::vector<std::string> dirs;
  std::vector<std::string> files;

  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == ".")
      continue;
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += '/';
    fullPath += name;

    struct stat st;
    if (stat(fullPath.c_str(), &st) != 0)
      continue;

    if (S_ISDIR(st.st_mode))
      dirs.push_back(name);
    else
      files.push_back(name);
  }
  closedir(dir);

  std::sort(dirs.begin(), dirs.end());
  std::sort(files.begin(), files.end());

  std::ostringstream html;

  html << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "<head>\n"
       << "  <meta charset=\"UTF-8\">\n"
       << "  <title>Index of " << uri << "</title>\n"
       << "  <style>\n"
       << "    body { font-family: monospace; padding: 20px; }\n"
       << "    h1   { border-bottom: 1px solid #ccc; padding-bottom: 8px; }\n"
       << "    table{ border-collapse: collapse; width: 100%; }\n"
       << "    th   { text-align: left; padding: 6px 16px; border-bottom: 2px "
          "solid #ccc; }\n"
       << "    td   { padding: 4px 16px; }\n"
       << "    tr:hover { background: #f5f5f5; }\n"
       << "    a    { text-decoration: none; color: #0366d6; }\n"
       << "    a:hover { text-decoration: underline; }\n"
       << "    .dir { color: #6f42c1; }\n"
       << "  </style>\n"
       << "</head>\n"
       << "<body>\n"
       << "  <h1>Index of " << uri << "</h1>\n"
       << "  <table>\n"
       << "    <tr>\n"
       << "      <th>Name</th>\n"
       << "      <th>Last Modified</th>\n"
       << "      <th>Size</th>\n"
       << "    </tr>\n";

  // 5. Parent directory link (except if we're at root)
  if (uri != "/") {
    html << "    <tr>\n"
         << "      <td><a href=\"../\">../</a></td>\n"
         << "      <td>-</td>\n"
         << "      <td>-</td>\n"
         << "    </tr>\n";
  }

  // 6. Directories first
  for (size_t i = 0; i < dirs.size(); i++) {
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += '/';
    fullPath += dirs[i];

    struct stat st;
    stat(fullPath.c_str(), &st);

    html << "    <tr>\n"
         << "      <td class=\"dir\">"
         << "<a href=\"" << dirs[i] << "/\">" << dirs[i] << "/</a>"
         << "</td>\n"
         << "      <td>" << formatTime(st.st_mtime) << "</td>\n"
         << "      <td>-</td>\n"
         << "    </tr>\n";
  }

  // 7. Files
  for (size_t i = 0; i < files.size(); i++) {
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += '/';
    fullPath += files[i];

    struct stat st;
    stat(fullPath.c_str(), &st);

    html << "    <tr>\n"
         << "      <td>"
         << "<a href=\"" << files[i] << "\">" << files[i] << "</a>"
         << "</td>\n"
         << "      <td>" << formatTime(st.st_mtime) << "</td>\n"
         << "      <td>" << formatSize(st.st_size) << "</td>\n"
         << "    </tr>\n";
  }

  html << "  </table>\n"
       << "</body>\n"
       << "</html>\n";

  return html.str();
}

bool isMatch(const std::string &uri, const std::string &location) {

  if (uri.compare(0, location.size(), location) != 0)
    return false;
  if (uri.size() == location.size())
    return true;
  if (location[location.size() - 1] == '/')
    return true;
  return uri[location.size()] == '/';
}

#include <iostream>
void Client::findTargetLocation() {
  const std::vector<LocationConfig> &locations = _config.getLocations();
  std::string uri = getUri();

  for (size_t i = 0; i < locations.size(); i++) {
    // std::cout << "check location : " << locations[i].getPath() << std::endl;
    if (isMatch(uri, locations[i].getPath())) {
      if (_targetLocation.getPath().empty() ||
          locations[i].getPath().size() > _targetLocation.getPath().size()) {
        _targetLocation = locations[i];
      }
    }
  }
  // std::cout << "check location : " << _targetLocation.getPath() << std::endl;
}


std::string readFile(const std::string &filepath) {
  // Implement file reading logic here, e.g., open the file, read its contents
  // into a string, and return it
  std::string content;
  std::ifstream file(filepath.c_str());
  if (!file.is_open()) {
    std::cout << "Failed to open file: " << filepath << std::endl;
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  content = ss.str();
  if (content.empty()) {
    std::cout << "File is empty: " << filepath << std::endl;
  }
  return content;
  // ...
}

void Client::sendFile(const std::string &filepath) {
  std::cout << "Sending file: " << filepath << std::endl;
  std::string content = readFile(filepath);
  if (content.empty()) {
    sendError(404);
    return;
  }
  std::cout << "File content size: " << content.size() << " bytes" << std::endl;

  setStatusCode("200");
  setversion("HTTP/1.1");
  setStatusMessage("OK");
  setHeader("Content-Length", std::to_string(content.size()));
  setHeader("Content-Type", "text/html"); // You may want to determine
  setBody(content);
  std::cout<< "Response to send:\n" << getRawResponse() << std::endl;
  buildResponse();
}

void Client::redirection(const std::string &newLocation) {
  std::cout << "Redirecting to: " << newLocation << std::endl;
  setStatusCode("301");
  setversion("HTTP/1.1");
  setStatusMessage("Moved Permanently");
  setHeader("Location", newLocation);
  setHeader("Content-Length", "0");
  buildResponse();
}



void Client::handelGET() {

  findTargetLocation();
  std::string uri = getUri();
  std::string root = _targetLocation.getRoot();
  std::string index = _targetLocation.getIndex();
  std::string locationPath = _targetLocation.getPath();      // "/docs"
  std::string uriSuffix = uri.substr(locationPath.length()); // "/rrrt"
  std::string target = root + uriSuffix; // "./www/docs/rrrt"

  if (!uriSuffix.empty() && uriSuffix[0] != '/' &&
      root[root.length() - 1] != '/')
    target = root + "/" + uriSuffix;
  else if (!uriSuffix.empty() && uriSuffix[0] == '/' &&
           root[root.length() - 1] == '/')
    target = root + uriSuffix.substr(1);
  else
    target = root + uriSuffix;
  struct stat statBuf;
  std::cout << "Target path: " << target << std::endl;

  if (stat(target.c_str(), &statBuf) == -1) {
    sendError(404);
    return;
  }

  if (S_ISREG(statBuf.st_mode)) {
      sendFile(target);
      return;

  } else if (S_ISDIR(statBuf.st_mode)) {

    if (uri[uri.length() - 1] != '/') {
      redirection(uri + "/");
      return;
    }
    std::string indexPath = target + "/" + index;
    struct stat indexStat;
    if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
      sendFile(indexPath);
      return;
      
    } else {
      if (_targetLocation.getAutoindex()) {
        std::string autoIndexHtml = buildAutoIndex(target, uri);
        if (autoIndexHtml.empty()) {
          sendError(403);
          return;
        }
        setStatusCode("200");
        setversion("HTTP/1.1");
        setStatusMessage("OK");
        setHeader("Content-Length", std::to_string(autoIndexHtml.size()));
        setHeader("Content-Type", "text/html");
        setBody(autoIndexHtml);
        buildResponse();
      } else {
        sendError(403);
        return;
      }
    }
  } else {
    // socket, pipe, device... not servable
    // _code = 403;
    return;
  }
}

Client::Client(int fd, const Config &config) : _fd(fd), _config(config) {
  _complete = false;
  setConfig(config);
}

int Client::getFd() const { return _fd; }

const Config &Client::getConfig() const { return _config; }

size_t getContentLength(const std::string &headers) {
  size_t pos = headers.find("Content-Length:");
  if (pos == std::string::npos)
    return 0;
  pos += 15;
  while (pos < headers.size() && headers[pos] == ' ')
    pos++;
  return std::strtoul(headers.c_str() + pos, NULL, 10);
}
