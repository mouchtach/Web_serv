#include "utils.hpp"

std::string intToStr(int num) {
    std::ostringstream ss;
    ss << num;
    return ss.str();
}

std::string stripSemicolon(const std::string &s) {
    if (!s.empty() && s[s.size() - 1] == ';')
        return s.substr(0, s.size() - 1);
    throw std::runtime_error("Value must end with ';', got '" + s + "'");
}