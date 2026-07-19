#pragma once
#include "config.hpp"
#include "location.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

class ConfigParssing {
private:
    std::string                 _filename;
    std::string                 _content;
    std::vector<std::string>    _tokens;
    std::vector<Config>         _configs;

public:
    ConfigParssing(const std::string &filename);
    ~ConfigParssing();
    void    ReadConfig();
    void    removeComments();
    void    tokenize();
    void    parseConfig();
    void    validate();
    void    parseServer(size_t &i);
    std::vector<Config> getConfigs() const;
};
