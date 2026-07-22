#include "utils.hpp"
#include <stdexcept>
// headr for atoi()
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