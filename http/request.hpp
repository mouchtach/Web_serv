#pragma once
#include <string>
#include "httpexception.hpp"
#include <map>
#include <iostream>

class Request {
private:

    std::string _method;
    std::string _uri;
    std::string _version;
    std::string _buffer;
    std::map<std::string, std::string> _headers;
    std::string _body;

    bool _header_complete;
    bool _request_complete;
    bool has_content_length;
    size_t _content_length;
    size_t _max_body_size;
    std::string _token;

public:
    Request();
    ~Request();

    // setters
    void setMethod(const std::string &method);
    void set_max_body_size(size_t max_body_size);
    void setUri(const std::string &uri);
    void setVersion(const std::string &version);
    void setContentLength(size_t length);
    void set_request_complete(bool complete);
    void setToken(const std::string &token);
    
    
    
    // getters
    std::string getToken() const ;
    std::string getContentType() const ;
    const std::string &getMethod() const ;
    const std::string &getUri() const ;
    const std::string &getVersion() const ;
    const std::string &getBuffer() const ;
    const std::string &getBody() const ;
    size_t getContentLength() const ;
    size_t get_max_body_size() const ;



    void addheader(std::string &key, std::string &value);
    bool is_content_length_done() const ;
    bool parseHeader();
    void parseBody();
    void validateHeaders();
    void parse();
    bool hasBody() const;
    bool isRequestComplete() const ;
    bool is_header_complete() const ;
    bool is_request_complete() const ;
    bool hasContentLength() const ;
    void appendData(const char *data, size_t length);
    void parseRequestLine(const std::string &line);
    void parseHeaders(const std::string &headers);
};