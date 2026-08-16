
#pragma once
#include <exception>
#include <string>

class redirectException : public std::exception {
private:
    std::string _redirectUrl;
    int _statusCode;
public:
    redirectException(const int status_code, const std::string &new_path);
    virtual ~redirectException() throw();
    const char* what() const throw();
    const std::string& getRedirectUrl() const;
    const int& getStatusCode() const;
};