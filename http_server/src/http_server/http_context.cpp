#include "http_context.h"
#include <sstream>
#include <iostream>

namespace http
{
    void HttpContext::print()
    {
        std::cout << "===== HttpRequest =====\n";

        std::cout << "method: ";

        switch (httpRequest_.method_)
        {
        case HttpRequest::kGet:
            std::cout << "GET";
            break;
        case HttpRequest::kPost:
            std::cout << "POST";
            break;
        case HttpRequest::kHead:
            std::cout << "Head";
            break;
        case HttpRequest::kPut:
            std::cout << "PUT";
            break;
        case HttpRequest::kDelete:
            std::cout << "DELETE";
            break;
        case HttpRequest::kOptions:
            std::cout << "OPTIONS";
            break;
        default:
            std::cout << "INVALID";
            break;
        }
        std::cout << '\n';

        std::cout << "path: " << httpRequest_.path_ << '\n';
        std::cout << "version: " << httpRequest_.version_ << '\n';
        std::cout << "contentLength: " << httpRequest_.contentLength_ << '\n';
        std::cout << "receive_time: " << httpRequest_.receive_time_.toString() << '\n';

        std::cout << "\nqueryParameters:\n";
        if (httpRequest_.queryParameters_.empty())
        {
            std::cout << "  (empty)\n";
        }
        else
        {
            for (const auto &[key, value] : httpRequest_.queryParameters_)
            {
                std::cout << "  " << key << " = " << value << '\n';
            }
        }

        std::cout << "\nheaders:\n";
        if (httpRequest_.headers_.empty())
        {
            std::cout << "  (empty)\n";
        }
        else
        {
            for (const auto &[key, value] : httpRequest_.headers_)
            {
                std::cout << "  " << key << ": " << value << '\n';
            }
        }

        std::cout << "\nbody:\n";
        if (httpRequest_.body_.empty())
        {
            std::cout << "  (empty)\n";
        }
        else
        {
            std::cout << httpRequest_.body_ << '\n';
        }

        std::cout << "=======================\n";
    }
    void HttpContext::clear()
    {
        state_ = kExpectRequestLine;
        HttpRequest tmp(httpRequest_.conn_,httpRequest_.httpServerPtr_);
        httpRequest_.swap(&tmp);
    }
    bool HttpContext::ParseRequest(muduo::net::Buffer *buffer, muduo::Timestamp timestamp)
    {
        httpRequest_.receive_time_ = timestamp;
        bool hasMore = true;
        while (hasMore)
        {
            if (state_ == kExpectRequestLine)
            {
                const char *pos = buffer->findCRLF();
                if (pos)
                {
                    std::string requestLine(buffer->peek(), pos);
                    if (parseRequestLine(requestLine) == false)
                        return false;
                    buffer->retrieve(requestLine.size() + 2);
                    state_ = kExpectRequestHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == kExpectRequestHeaders)
            {

                while (true)
                {
                    const char *pos = buffer->findCRLF();
                    if (!pos)
                        hasMore = false;
                    std::string header(buffer->peek(), pos);
                    buffer->retrieve(header.size() + 2);
                    if (header.size() == 0)
                    {
                        state_ = kExpectRequestBody;
                        break;
                    }
                    if (setHeader(header) == false)
                        return false;
                    
                }
            }
            else if (state_ == kExpectRequestBody)
            {
                if (httpRequest_.headers_.count("Content-Length"))
                    httpRequest_.contentLength_ = std::stol(httpRequest_.headers_["Content-Length"]);
                if (buffer->readableBytes() >= httpRequest_.contentLength_)
                {
                    httpRequest_.body_ = std::string(buffer->peek(), httpRequest_.contentLength_);
                    buffer->retrieve(httpRequest_.contentLength_);
                    state_ = kGotAll;
                }
                else
                {
                    hasMore = false;
                }
            }
            else if (state_ == kGotAll)
            {
                hasMore = false;
            }
        }

        return true;
    }
    bool HttpContext::parseRequestLine(const std::string &requestLine)
    {
        std::istringstream iss(requestLine);
        std::string method;
        std::string path;
        std::string version;
        if (!(iss >> method >> path >> version))
            return false;
        if (setMethod(method) == false ||
            setPath(path) == false ||
            setVersion(version) == false)
            return false;
        return true;
    }
    bool HttpContext::setMethod(const std::string &method)
    {
        if (method == "GET")
            httpRequest_.method_ = HttpRequest::kGet;
        else if (method == "POST")
            httpRequest_.method_ = HttpRequest::kPost;
        else if (method == "HEAD")
            httpRequest_.method_ = HttpRequest::kHead;
        else if (method == "PUT")
            httpRequest_.method_ = HttpRequest::kPut;
        else if (method == "DELETE")
            httpRequest_.method_ = HttpRequest::kDelete;
        else if (method == "OPTIONS")
            httpRequest_.method_ = HttpRequest::kOptions;
        else
            return false;
        return true;
    }
    bool HttpContext::setPath(const std::string &path)
    {
        auto setQueryParameters = [&](const std::string &Query) -> bool
        {
            size_t pos = Query.find('=');
            if (pos == std::string::npos)
                return false;
            std::string &&key = Query.substr(0, pos);
            std::string &&value = Query.substr(pos + 1);
            if (key.size() == 0 || value.size() == 0)
                return false;
            if (httpRequest_.queryParameters_.count(key))
                return false;
            httpRequest_.queryParameters_[key] = value;
            return true;
        };
        auto pos = path.find('?');
        if (pos == 0)
            return false;
        httpRequest_.path_ = path.substr(0, pos);
        if (pos != std::string::npos)
        {
            size_t left = 0;
            size_t right = pos;
            while (true)
            {
                left = right + 1;
                right = path.find('&', left);
                if (setQueryParameters(path.substr(left, right-left)) == false)
                    return false;
                if (right == std::string::npos)
                {
                    break;
                }
            }
        }
        return true;
    }
    bool HttpContext::setVersion(const std::string &version)
    {
        auto pos = version.find('/');
        if (pos == std::string::npos || pos + 1 == version.size())
            return false;
        httpRequest_.version_ = version.substr(pos + 1);
        return true;
    }

    // delete blank char
    static std::string trim(const std::string &s)
    {
        size_t begin = 0;
        while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
            ++begin;

        size_t end = s.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;

        return s.substr(begin, end - begin);
    }

    bool HttpContext::setHeader(const std::string &header)
    {
        size_t pos = header.find(':');
        if (pos == std::string::npos)
            return false;

        std::string key = trim(header.substr(0, pos));
        std::string value = trim(header.substr(pos + 1));

        if (key.empty())
            return false;

        if (httpRequest_.headers_.count(key))
            return false;

        httpRequest_.headers_[key] = value;
        return true;
    }

}