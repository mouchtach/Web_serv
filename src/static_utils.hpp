// src/static_utils.hpp
#pragma once
#include <string>

std::string intToStr(int num);
int strToInt(const std::string &s);
std::string stripSemicolon(const std::string &s);
void  toLowerCase(std::string &s);
std::string takenamefile(const std::string &body);
std::string readFile(const std::string &filepath);
std::string getMimeType(const std::string &path);
std::string appendPath(const std::string &root, const std::string &path);
bool isPathSafe(const std::string &uri);
std::string buildAutoIndex(const std::string &dirPath, const std::string &uri);
std::string getBoundary(const std::string &contentType);
std::string takeBodyContent(const std::string &body, const std::string &boundary);