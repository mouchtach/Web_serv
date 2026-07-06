#pragma once
#include "serverConfig.hpp"

class LocationConfig : public serverConfig 
{
private:
    std::string _path;
    std::string _targetPath;
    bool rootOverridden;
    bool redc;
    bool CGI;
    std::pair<int , std::string> _return;
    std::string _cgiExtension;
    std::string _cgiPath;


public:
    LocationConfig();
    LocationConfig(const serverConfig& parentConfig) : serverConfig(parentConfig), _path("") ,  rootOverridden(false) , redc(false), CGI(false){}
    virtual ~LocationConfig();
    bool hasredirection() { return redc;}
    void override(const std::vector<std::string> &tokens, size_t &i, const std::string path);
    void setReturn(const std::vector<std::string>& tokens, size_t* i);
    void setPath(const std::string& path);
    void setRootOverridden(bool t) { rootOverridden = t; }
    void setCgi(bool t) { CGI = t; }
    void setCgiExtension(const std::string& ext);
    void setCgiPath(const std::string& path);

    const std::string& getPath() const;
    const std::pair<int, std::string>& getReturn() const;
    const std::string& getCgiExtension() const;
    const std::string& getCgiPath() const;
    bool isMethodAllowed(const int method) const;
    void error(const std::string& message) const {
        throw std::runtime_error("LocationConfig: " + message);
    }
    void print() const;
    bool isRootOverridden() const { return rootOverridden; }
    bool isCgi() const { return CGI; }
};