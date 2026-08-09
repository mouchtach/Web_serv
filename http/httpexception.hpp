#pragma once
#include <exception>
#include <string>


class HttpException : public std::exception {
    private:
    int _statusCode;
    std::string _message;
public:
    HttpException(int statusCode, const std::string& message);
    int getStatusCode() const;

    const std::string& getMessage() const;
    virtual ~HttpException() throw();
    virtual const char* what() const throw();
};