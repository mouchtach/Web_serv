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
void Request::parseHeader() {
    size_t headerEnd = _buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return;
    _header_complete = true;
    std::string headerPart = _buffer.substr(0, headerEnd);
    size_t firstLineEnd = headerPart.find("\r\n");
    if (firstLineEnd == std::string::npos)
        throw HttpException(400, "bad request");
    parseRequestLine(headerPart.substr(0, firstLineEnd));
    parseHeaders(headerPart.substr(firstLineEnd + 2));
    //body part starts after the header
    // appendBody(_buffer.substr(headerEnd + 4));
    _buffer.erase(0, headerEnd + 4); // Remove the header part from the buffer
    std::cout << "Parsed request line: " << _method << " " << _uri << " " << _version << std::endl;
    

    // check if metod is POST and if content-length is present in headers
    if (_method == "POST" && _headers.find("Content-Length") == _headers.end())
        throw HttpException(400, "bad request");{
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