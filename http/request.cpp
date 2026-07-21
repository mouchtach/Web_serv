#include "request.hpp"

Request::Request() : _header_complete(false), _request_complete(false), has_content_length(false) {}

Request::~Request() {}


void Request::setMethod(const std::string &method) {
    if (method != "GET" && method != "POST" && method != "DELETE")
        throw HttpException(400, "bad request");
    _method = method;
}
void Request::setUri(const std::string &uri) {
    if (uri.empty())
        throw HttpException(400, "bad request");
    _uri = uri;
}

void Request::setVersion(const std::string &version) {
    if (version != "HTTP/1.1" && version != "HTTP/1.0")
        throw HttpException(400, "bad request");
    _version = version;
}

bool Request::is_header_complete() const {
    return _header_complete;
}
bool Request::is_request_complete() const {
    return _request_complete;
}

void Request::appendData(const char *data, size_t length) {
    _buffer.append(data, length);
}

void Request::addheader(const std::string &key, const std::string &value) {
    _headers[key] = value;
}
// void Request::appendBody(const std::string &body) {
//     _body += body;
//     if (_body.size() >= _content_length) {
//         re = true;
//     }
// }

void Request::setContentLength(unsigned long length) {
    _content_length = length;
}


#include <iostream>

void Request::parseBody()
{
    _body.append(_buffer);
    _buffer.clear();

    if (_body.size() >= _content_length)
    {
        _body.resize(_content_length);
        _request_complete = true;
    }
}

bool Request::parseHeader()
{
    size_t headerEnd = _buffer.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
        return false;

    std::string header = _buffer.substr(0, headerEnd);

    size_t firstLine = header.find("\r\n");

    if (firstLine == std::string::npos)
        throw HttpException(400, "Bad Request");

    parseRequestLine(header.substr(0, firstLine));
    parseHeaders(header.substr(firstLine + 2));

    validateHeaders();

    _buffer.erase(0, headerEnd + 4);

    _header_complete = true;

    return true;
}

void Request::validateHeaders()
{
    if (_method == "POST" && !has_content_length)
        throw HttpException(400, "Bad Request");

    if (has_content_length &&
        _content_length > _max_body_size)
        throw HttpException(413, "Payload Too Large");
}

bool Request::hasBody() const
{
    return has_content_length;
}

void Request::parse() {
    
    if (!_header_complete)
    {
        if (!parseHeader())
            return;
    }
    if (hasBody())
    {
        parseBody();
        if (!_request_complete)
            return;
    }
    else
    {
        _request_complete = true;
    }
}

void Request::parseRequestLine(const std::string &line) {
    size_t pos1 = line.find(' ');
    if (pos1 == std::string::npos)
        throw HttpException(400, "bad request" );
    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string::npos)
        throw HttpException(400, "bad request" );
    std::string method =line.substr(0, pos1);
    setUri(line.substr(pos1 + 1,pos2 - pos1 - 1));
    setVersion(line.substr(pos2 + 1));
    setMethod(method);
}

void Request::parseHeaders(const std::string &headers) {
    size_t start = 0;
    while (start < headers.size()) {
        size_t end = headers.find("\r\n", start);
        if (end == std::string::npos)
            end = headers.size();
        std::string line = headers.substr(start, end - start);
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // set content length header to the value of content-length
            if (key == "Content-Length") {
                setContentLength(std::stoul(value));
                has_content_length = true;
            }
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            addheader(key, value);
        }
        start = end + 2; // Move past "\r\n"
    }
}