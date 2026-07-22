#pragma once
#include <exception>
#include <string>


class HttpException : public std::exception {
public:
    HttpException(int statusCode, const std::string& message) : _statusCode(statusCode), _message(message) {}
    int getStatusCode() const {return _statusCode; }

    const std::string& getMessage() const {
        return _message;
    }
    virtual ~HttpException() throw() {}
    virtual const char* what() const throw() {
        return _message.c_str();
    }

private:
    int _statusCode;
    std::string _message;
};