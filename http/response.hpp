#pragma once
#include <string>
#include <map>
#include <sstream>
#include <ctime>

class Response {
private:
    std::string _version;
    int         _statusCode;
    std::string _statusMessage;
    std::string _body;
    std::map<std::string, std::string> _headers;
    std::string _rawResponse;
    size_t      _sentBytes;

    std::string intToStr(int n) const {
        std::ostringstream ss;
        ss << n;
        return ss.str();
    }

public:
    Response();
    ~Response();

    void setVersion(const std::string &version) { _version = version; }
    void setStatusCode(int code, const std::string &message) {
        _statusCode = code;
        _statusMessage = message;
    }
    void setHeader(const std::string &key, const std::string &value) {
        _headers[key] = value;
    }
    void setBody(const std::string &body) { _body = body; }

    void buildResponse();

    const std::string &getRawResponse() const { return _rawResponse; }
    int getStatusCode() const { return _statusCode; }

    // send-loop bookkeeping (used by polloutprocess)
    void setSentBytes(size_t n) { _sentBytes = n; }
    size_t getSentBytes() const { return _sentBytes; }
    void addBytesSent(size_t n) { _sentBytes += n; }
    bool isFullySent() const { return _sentBytes >= _rawResponse.size(); }
    void resetSendState() { _sentBytes = 0; }

    std::string getCurrentDate() const {
        char buffer[100];
        std::time_t now = std::time(NULL);
        std::tm *gmt = std::gmtime(&now);
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return std::string(buffer);
    }

    // generic fallback error page builder (used outside CGI too, e.g. 404/403/500)
    void sendError(int code, const std::string &message, const std::string &customBody = "");
};