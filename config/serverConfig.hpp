#pragma once
#include <map>
#include <string>
#include <vector>

class serverConfig 
{
protected:
    std::string _root;
    std::string _index;
    bool _autoindex;
    size_t   _clientMaxBodySize;
    std::map<int, std::string> _errorPages;
    std::vector<std::string> _methods;
    std::string _cgiExtension;
    std::string _cgiPath;

public:
    serverConfig ();
    virtual ~serverConfig ();

    void setRoot(const std::string& root);
    void setIndex(const std::string& index);
    void setAutoindex(const std::string& autoindex);
    void setClientMaxBodySize(const std::string& size);
    void addErrorPage(const std::string& code, const std::string& path);
    void setMethods(const std::vector<std::string>& methods);
    void setCgiExtension(const std::string& ext);
    void setCgiPath(const std::string& path);
    // void setReturn(const std::vector<std::string>& tokens, size_t* i);

    const std::string& getRoot() const;
    const std::string& getIndex() const;
    bool getAutoindex() const;
    size_t getClientMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::vector<std::string>& getMethods() const;
    const std::string& getCgiExtension() const;
    const std::string& getCgiPath() const;
    // const std::pair<int, std::string>& getReturn() const;
    std::string stripSemicolon(const std::string &s);
    std::string intToStr(int n);
};