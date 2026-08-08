// src/static_utils.cpp
#include "static_utils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

std::string intToStr(int num) {
    std::ostringstream ss;
    ss << num;
    return ss.str();
}

int strToInt(const std::string &s) {
    // check if digits only
    for (size_t i = 0; i < s.size(); ++i)
        if (!isdigit(s[i]))
            throw std::runtime_error("Value must be a number, got '" + s + "'");
    return atoi(s.c_str());
}

std::string stripSemicolon(const std::string &s) {
    if (!s.empty() && s[s.size() - 1] == ';')
        return s.substr(0, s.size() - 1);
    throw std::runtime_error("Value must end with ';', got '" + s + "'");
}

void  toLowerCase(std::string &s) {
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = std::tolower(s[i]);
}

std::string takenamefile(const std::string &body) {
    // Find the filename in the body
    std::string filename;
    size_t pos = body.find("filename=\"");
    if (pos != std::string::npos) {
        pos += 10; // Move past 'filename="'
        size_t endPos = body.find("\"", pos);
        if (endPos != std::string::npos) {
            filename = body.substr(pos, endPos - pos);
        }
    }
    return filename;
}

std::string readFile(const std::string &filepath) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string getMimeType(const std::string &path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(pos);
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png")  return "image/png";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".pdf")  return "application/pdf";
    if (ext == ".json") return "application/json";
    return "application/octet-stream";
}

std::string appendPath(const std::string &root, const std::string &path) {
    if (root.empty()) return path;
    if (!root.empty() && root[root.size()-1] == '/' && !path.empty() && path[0] == '/')
        return root + path.substr(1);
    if (!root.empty() && root[root.size()-1] != '/' && !path.empty() && path[0] != '/')
        return root + "/" + path;
    return root + path;
}

bool isPathSafe(const std::string &uri) {
    if (uri.find("..") != std::string::npos) return false;
    if (uri.find("//") != std::string::npos)  return false;
    return true;
}

std::string buildAutoIndex(const std::string &dirPath, const std::string &uri) {
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) return "";

    std::vector<std::string> dirs, files;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        std::string fullPath = appendPath(dirPath, name);
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) dirs.push_back(name);
        else files.push_back(name);
    }
    closedir(dir);
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head><title>Index of " << uri
         << "</title></head><body><h1>Index of " << uri << "</h1><table>";
    if (uri != "/")
        html << "<tr><td><a href=\"../\">../</a></td></tr>";
    for (size_t i = 0; i < dirs.size(); ++i)
        html << "<tr><td><a href=\"" << dirs[i] << "/\">" << dirs[i] << "/</a></td></tr>";
    for (size_t i = 0; i < files.size(); ++i)
        html << "<tr><td><a href=\"" << files[i] << "\">" << files[i] << "</a></td></tr>";
    html << "</table></body></html>";
    return html.str();
}

std::string getBoundary(const std::string &contentType)
{
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos)
        return "";

    return contentType.substr(pos + 9);
}


std::string takeBodyContent(const std::string &body, const std::string &boundary)
{
    size_t start = body.find("\r\n\r\n");
    if (start == std::string::npos)
        return "";

    start += 4;

    std::string delimiter = "\r\n--" + boundary;
    size_t end = body.find(delimiter, start);
    if (end == std::string::npos)
        return "";
    return body.substr(start, end - start);
}