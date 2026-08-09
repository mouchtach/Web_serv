#include "response.hpp"
#include "../src/static_utils.hpp"

Response::Response()
    : _version("HTTP/1.0"), _statusCode(200), _statusMessage("OK"), _sentBytes(0) {
}

Response::~Response() {
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