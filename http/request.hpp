#pragma once

#include <string>
#include "httpexception.hpp"
#include <map>

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
    unsigned long _content_length;
    unsigned long _max_body_size; 

public:
    Request();
    ~Request();


    void setMethod(const std::string &method);
    void set_max_body_size(unsigned long max_body_size) { _max_body_size = max_body_size; }
    void setUri(const std::string &uri);
    void setVersion(const std::string &version);
    void addheader(const std::string &key, const std::string &value);
    void setContentLength(unsigned long length);
    void set_request_complete(bool complete) { _request_complete = complete; }
    bool is_content_length_done() const { return _body.size() >= _content_length; }
    bool parseHeader();
    void parseBody();
    void validateHeaders();
    void parse();
    bool hasBody() const;
    bool isRequestComplete() const { return _request_complete; }
    // getters
    const std::string &getMethod() const { return _method; }
    const std::string &getUri() const { return _uri; }
    const std::string &getVersion() const { return _version; }
    const std::map<std::string, std::string> &getHeaders() const { return _headers; }
    const std::string &getBody() const { return _body; }
    const std::string &getBuffer() const { return _buffer; }
    unsigned long getContentLength() const { return _content_length; }  




    bool is_header_complete() const ;
    bool is_request_complete() const ;
    bool hasContentLength() const { return has_content_length; }
    void appendData(const char *data, size_t length);
    void parseRequestLine(const std::string &line);
    void parseHeaders(const std::string &headers);
    // void parse();
// 
};