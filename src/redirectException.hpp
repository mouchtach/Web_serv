
#pragma once
#include <exception>
#include <string>

class redirectException : public std::exception {
private:
    std::string _redirectUrl;
public:
    redirectException(const std::string &new_path) : _redirectUrl(new_path) {}
    virtual ~redirectException() throw() {}
    const char* what() const throw() {
        return "Redirect Exception";
    }
    const std::string& getRedirectUrl() const {
        return _redirectUrl;
    }
};