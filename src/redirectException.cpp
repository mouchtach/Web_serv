#include "redirectException.hpp"

redirectException::redirectException(const std::string &new_path) : _redirectUrl(new_path) {}

redirectException::~redirectException() throw() {}

const char* redirectException::what() const throw() {
    return "Redirect Exception";
}

const std::string& redirectException::getRedirectUrl() const {
    return _redirectUrl;
}
