#pragma once
#include <string>
#include <fstream>
#include <map>
    #include <iostream>

#include <sstream>
#include "../config/Config.hpp"

class Response {
private:
    std::string _rawResponse;
    Config _config;
    std::string _version;
    std::string _body;
    size_t _contentLength;
    std::map<std::string, std::string> _header;
    std::string contentType;
    std::string _statusCode;
    std::string _statusMessage;
    size_t _sentBytes;

public:
    Response() : _rawResponse(""), _sentBytes(0) {};
    void setConfig(const Config &config) { _config = config; }

    std::string statusCodeString(int code) {
        switch (code) {
            case 200: return "OK";
            case 301: return "Moved Permanently";
            case 400: return "Bad Request";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 413: return "Payload Too Large";
            case 500: return "Internal Server Error";
            case 502: return "Bad Gateway";
            case 501: return "Not Implemented";
            case 505: return "HTTP Version Not Supported";
            default: return "Unknown Status";
        }
    }
    
    std::string buildErrorPage(int code, std::string message)
    {
        std::string codeStr = std::to_string(code);
        std::string html =
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>" + codeStr + " " + message + "</title></head>\n"
            "<body>\n"
            "<h1>" + codeStr + " " + message + "</h1>\n"
            "<hr>\n"
            "<p>webserv</p>\n"
            "</body>\n"
            "</html>\n";
        return html;
    }


    std::string getErrorBody(int code) {
        std::map<int, std::string> errorPages = _config.getErrorPages();
        if (errorPages.find(code) != errorPages.end()) {
            std::string path = errorPages[code];
            std::ifstream file(path.c_str());
            if (file.is_open()) {
                std::ostringstream ss;
                ss << file.rdbuf();
                return ss.str();
            }
        }
        return buildErrorPage(code, statusCodeString(code));
    }

    void sendError(int code)
    {
        std::string body = getErrorBody(code);
        _statusCode = std::to_string(code);
        _statusMessage = statusCodeString(code);
        _version = "HTTP/1.0";
        _header["Content-Length"] = std::to_string(body.size());
        _header["Content-Type"] = "text/html";
        _body = body;
        buildResponse();
    }

    // void responseprocces();

    #include <ctime>

    std::string getCurrentDate()
    {
        char buffer[100];

        std::time_t now = std::time(NULL);
        std::tm* gmt = std::gmtime(&now);
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return buffer;
    }
    
    void setRawResponse(const std::string& response) {
        _rawResponse = response;
    }
    void setContentLength(size_t length) {
        _contentLength = length;
    }
    size_t getContentLength() const {
        return _contentLength;
    }
    void setContentType(const std::string& type) {
        contentType = type;
    }
    const std::string& getContentType() const {
        return contentType;
    }   
    const std::string& getRawResponse() const {
        return _rawResponse;
    }
    void setStatusCode(const std::string& code) {
        _statusCode = code;
    }
    void setStatusMessage(const std::string& message) {
        _statusMessage = message;
    }
    void setHeader(const std::string& key, const std::string& value) {
        _header[key] = value;
    }
    void setBody(const std::string& body) {
        _body = body;
    }
    void setSentBytes(size_t bytes) { _sentBytes = bytes; }
    size_t getSentBytes() const { return _sentBytes; }
    void addBytesSent(size_t n) { _sentBytes += n; }
    bool isFullySent() const { return _sentBytes >= _rawResponse.size(); }
    void resetSendState() { _sentBytes = 0; }
    void setversion(const std::string& version) {
        _version = version;
    }
    void buildResponse() {
        _sentBytes = 0;
        _header["Date"] =  getCurrentDate();
        _header["Server"] = "webserv/1.0";
        _rawResponse = _version + " " + _statusCode + " " + _statusMessage + "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = _header.begin(); it != _header.end(); ++it) {
            _rawResponse += it->first + ": " + it->second + "\r\n";
        }
        _rawResponse += "\r\n" + _body;
    }
};