#pragma once

#include "config.hpp"

class Location : public Config {

private:
    std::string _path;
    std::string _targetPath;
    bool rootOverridden;
    bool redc;
    std::pair<int , std::string> _return;
    std::string _cgiExtension;
    std::string _cgiPath;

public:
    Location();
    ~Location();
    Location(const Location &other);
    Location(const Config &other);
    Location &operator=(const Location &other);

    void overrideLocation(const std::vector<std::string> &tokens, size_t &i);
    void setPath(const std::string &path);
    void setTargetPath(const std::string &targetPath);
    void setRootOverridden(bool overridden);
    void setRedc(bool redc);
    void setReturn(const std::pair<int, std::string> &ret);
    void setCgiExtension(const std::string &cgiExtension);
    void setCgiPath(const std::string &cgiPath);
    
    const std::string &getPath() const;
    const std::string &getTargetPath() const;
    bool isRootOverridden() const;
    bool isRedc() const;
    const std::pair<int, std::string> &getReturn() const;
    const std::string &getCgiExtension() const;
    const std::string &getCgiPath() const;
};

