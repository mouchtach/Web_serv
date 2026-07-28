// src/static_utils.hpp
#pragma once
#include <string>

std::string readFile(const std::string &filepath);
std::string getMimeType(const std::string &path);
std::string appendPath(const std::string &root, const std::string &path);
bool isPathSafe(const std::string &uri);
std::string buildAutoIndex(const std::string &dirPath, const std::string &uri);