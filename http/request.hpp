#pragma once
#include <string>
#include <map>
#include <iostream>
enum HttpMethod
{
    GET,
    POST,
    DELETE,
    UNKNOWN
};

class Request
{
private:
    std::string _rawRequest;
    HttpMethod _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    size_t _contentLength;
    
    void parseRequestLine(const std::string& line);
    void parseHeaders(const std::string& headersPart);
    void parseBody(const std::string& bodyPart);
    
public:

    bool _complete;
    void setComplete(bool complete) { _complete = complete; }
    bool isComplete() const { return _complete; }
    Request() : _rawRequest(""), _complete(false) {};
    void appendrequest(const std::string& data) {
        _rawRequest.append(data);
    }

    HttpMethod getMethod() const {
        return _method;
    };
    const std::string& getUri() const{
        return _uri;
    };
    void clear_rawRequest() {
        _rawRequest.clear();
    }
    std::string getRawRequest() const {
        return _rawRequest;
    }
    const std::string& getVersion() const;
    const std::string& getBody() const;
    const std::map<std::string, std::string>& getHeaders() const;
    // std::string getHeader(const std::string& key) const;
    size_t getContentLength() const { return _contentLength; }
    // void displayrequest();
    bool validkey(const std::string& key) const;

    void parseRequest();
    HttpMethod stringToMethod(const std::string& method);
    bool isheaderComplete();
    bool isRequestComplete();
    // bool hasHeader(const std::string& key) const;
};
