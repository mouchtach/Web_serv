#pragma once
#include "serverConfig.hpp"

class LocationConfig : public serverConfig 
{
private:
    std::string _path;
    std::string _targetPath;
    bool rootOverridden;
    bool redc;
    std::pair<int , std::string> _return;


public:
    LocationConfig();
    LocationConfig(const serverConfig& parentConfig) : serverConfig(parentConfig), _path("") ,  rootOverridden(false) , redc(false){}
    virtual ~LocationConfig();
    bool hasredirection() { return redc;}
    void override(const std::vector<std::string> &tokens, size_t &i, const std::string path);
    void setReturn(const std::vector<std::string>& tokens, size_t* i);
    void setPath(const std::string& path);
    void setRootOverridden(bool t) { rootOverridden = t; }

    const std::string& getPath() const;
    const std::pair<int, std::string>& getReturn() const;
    bool isMethodAllowed(const int method) const;
    void error(const std::string& message) const {
        throw std::runtime_error("LocationConfig: " + message);
    }
    void print() const;
    bool isRootOverridden() const { return rootOverridden; }
};