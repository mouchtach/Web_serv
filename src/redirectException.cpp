#include "redirectException.hpp"

redirectException::redirectException(const int status_code, const std::string &new_path) : _redirectUrl(new_path), _statusCode(status_code) {}

redirectException::~redirectException() throw() {}

const char* redirectException::what() const throw() {
    return "Redirect Exception";
}

const std::string& redirectException::getRedirectUrl() const {
    return _redirectUrl;
}

const int& redirectException::getStatusCode() const {
    return _statusCode;
}
