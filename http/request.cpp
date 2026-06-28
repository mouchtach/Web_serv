#include "request.hpp"


HttpMethod Request::stringToMethod(const std::string& method){

    if (method == "GET")
        return GET;
    else if (method == "POST")
        return POST;
    else if (method == "DELETE")
        return DELETE;
    else
        return UNKNOWN;
}

void Request::parseRequestLine(const std::string& line)
{
    size_t pos1 = line.find(' ');

    if (pos1 == std::string::npos)
        throw 400;

    size_t pos2 = line.find(' ', pos1 + 1);

    if (pos2 == std::string::npos)
        throw 400;

    std::string method =line.substr(0, pos1);

    _uri = line.substr(pos1 + 1,pos2 - pos1 - 1);

    _version = line.substr(pos2 + 1);

    _method = stringToMethod(method);

    if (_method == UNKNOWN)
        throw 400;
    if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
        throw 400;
    if (_uri.empty())
        throw 400;
}

bool Request::validkey(const std::string& key) const
{
    if (key.empty())
        return false;
    if (key.find(' ') != std::string::npos)
        return false;
    return true;
}

void Request::parseHeaders(const std::string& headersPart)
{
    size_t start = 0;

    while (start < headersPart.size())
    {
        size_t end = headersPart.find("\r\n", start);
        if (end == std::string::npos)
            end = headersPart.size();
        std::string line = headersPart.substr(start,end - start);

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            throw 400;

        std::string key = line.substr(0, colon);
        // i want changer the key to lowercase
        std::string value = line.substr(colon + 1);
        // Trim leading spaces from value
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (validkey(key))
        {
            if (key == "content-length")
            {
                _contentLength = std::strtoul(value.c_str(), NULL, 10);
            }
            _headers[key] = value;
        }
        else
            throw 400;
        start = end + 2;
    }
}

void Request::parseBody(const std::string& bodyPart)
{
    _body = bodyPart;
}



bool Request::isheaderComplete()
{
    if(!_complete)
    {
        size_t pos = _rawRequest.find("\r\n\r\n");
        if (pos != std::string::npos)
        {
            _complete = true;        
            parseRequest();
        }
    }
    return _complete;
}


void Request::parseRequest()
{
    size_t headerEnd = _rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        throw 400;
    std::string headerPart = _rawRequest.substr(0, headerEnd);
    size_t firstLineEnd = headerPart.find("\r\n");
    if (firstLineEnd == std::string::npos)
        throw 400;
    parseRequestLine(headerPart.substr(0, firstLineEnd));
    parseHeaders(headerPart.substr(firstLineEnd + 2));
    _rawRequest.erase(0, headerEnd + 4);
}

bool Request::isRequestComplete()
{
    if(_method == POST)
    {
        if (_rawRequest.size() >= getContentLength())
        {
            _body = _rawRequest.substr(0, getContentLength());
            // std::cout  << "Request body complete based on Content-Length header." << std::endl;
            return true;
        }
        else 
        {
            // std::cout  << "Request body incomplete. Received " << _rawRequest.size() << " bytes, expected " << getContentLength() << " bytes." << std::endl;
            return false;
        }
    }
    return true; 
}

const std::string& Request::getBody() const
{
    return _body;
}