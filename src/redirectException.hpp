
#pragma once
#include <exception>
#include <string>

class redirectException : public std::exception {
private:
    std::string _redirectUrl;
public:
    redirectException(const std::string &new_path);
    virtual ~redirectException() throw();
    const char* what() const throw();
    const std::string& getRedirectUrl() const;
};