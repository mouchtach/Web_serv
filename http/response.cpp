#include "response.hpp"

Response::Response():  _statusCode(200), _version("HTTP/1.0"), _statusMessage("OK"), _sentBytes(0) {}

Response::~Response() {}

// Setters

void Response::setVersion(const std::string &version) { _version = version; }

void Response::setStatusCode(int code, const std::string &message) {
    _statusCode = code;
    _statusMessage = message;
}

void Response::setHeader(const std::string &key, const std::string &value) {
    _headers[key] = value;
}

void Response::setBody(const std::string &body) { _body = body; }

void Response::setErrorPages(const std::map<int, std::string> &errorPages) { _errorPages = errorPages; }

void Response::setSentBytes(size_t n) { _sentBytes = n; }


// Getters
int Response::getStatusCode() const { return _statusCode; }

size_t Response::getSentBytes() const { return _sentBytes; }

const std::string& Response::getRawResponse() const { return _rawResponse; }

std::string Response::getCurrentDate() const {
        char buffer[100];
        std::time_t now = std::time(NULL);
        std::tm *gmt = std::gmtime(&now);
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return std::string(buffer);
}


void Response::buildResponse() {
    _sentBytes = 0;
    if (_headers.find("Date") == _headers.end())
        _headers["Date"] = getCurrentDate();
    if (_headers.find("Server") == _headers.end())
        _headers["Server"] = "webserv/1.0";
    if (_headers.find("Content-Length") == _headers.end())
        _headers["Content-Length"] = intToStr(_body.size());
    _rawResponse = _version + " " + intToStr(_statusCode) + " " + _statusMessage + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        _rawResponse += it->first + ": " + it->second + "\r\n";
    }
    _rawResponse += "\r\n" + _body;
}


void Response::addBytesSent(size_t n) { _sentBytes += n; }

bool Response::isFullySent() const { return _sentBytes >= _rawResponse.size(); }

void Response::resetSendState() { _sentBytes = 0; }

void Response::sendError(int code, const std::string &customBody) {
    std::string message = getStatusMessage(code);
    std::string body = customBody;
    if (body.empty()) {
        std::map<int, std::string>::const_iterator it = _errorPages.find(code);
        if (it != _errorPages.end())
            body = readFile(it->second);
    }
    if (body.empty()) {
        body =
            "<!DOCTYPE html>\n<html>\n<head><title>" + intToStr(code) + " " + message +
            "</title></head>\n<body>\n<h1>" + intToStr(code) + " " + message +
            "</h1>\n<hr>\n<p>webserv</p>\n</body>\n</html>\n";
    }
    setStatusCode(code, message);
    setHeader("Content-Type", "text/html");
    setBody(body);
    buildResponse();
}