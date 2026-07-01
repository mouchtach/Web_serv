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

private:
    int _statusCode;
    std::string _message;
};