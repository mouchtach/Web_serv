#pragma once
#include <string>

bool isMatch(const std::string &uri, const std::string &location);
std::string readFile(const std::string &filepath);
std::string formatSize(off_t size);
std::string formatTime(time_t t);
std::string buildAutoIndex(const std::string &dirPath, const std::string &uri);
std::string getMimeType(const std::string& path);
std::string appendPath(const std::string& root, const std::string& path);
std::string extractFileName(const std::string& body);
std::string extractBodyfile(const std::string& body);
bool isPathsafe(const std::string &uri);