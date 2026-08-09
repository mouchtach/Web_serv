#include "httpexception.hpp"


HttpException::HttpException(int statusCode, const std::string& message) : _statusCode(statusCode), _message(message) {
}

HttpException::~HttpException() throw() {
}

int HttpException::getStatusCode() const {
    return _statusCode;
}

std::string const &HttpException::getMessage() const {
    return _message;
}

const char* HttpException::what() const throw() {
    return _message.c_str();
}

