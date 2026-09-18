#pragma once
#include "../src/static_utils.hpp"
#include <string>
#include <map>
#include <sstream>
#include <ctime>


class Response {
private:

    int         _statusCode;
    std::string _version;
    std::string _statusMessage;
    std::string _body;
    std::map<std::string, std::string> _headers;
    std::string _rawResponse;
    size_t      _sentBytes;
    std::map<int, std::string> _errorPages; 

public:
    Response();
    ~Response();

    void setVersion(const std::string &version);
    void setStatusCode(int code, const std::string &message);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    void setErrorPages(const std::map<int, std::string> &errorPages);
    void setSentBytes(size_t n);

    
    int getStatusCode() const ;
    size_t getSentBytes() const ;
    const std::string &getRawResponse() const ;
    std::string getCurrentDate() const;
    
    void buildResponse();
    void addBytesSent(size_t n);
    bool isFullySent() const ;
    void resetSendState();
    void sendError(int code, const std::string &customBody = "");
};