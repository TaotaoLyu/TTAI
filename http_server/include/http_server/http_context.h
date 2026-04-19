#pragma once
#include "http_request.h"

namespace http
{
    class HttpContext
    {
        enum HttpRequestParseState
        {
            kExpectRequestLine,
            kExpectRequestHeaders,
            kExpectRequestBody,
            kGotAll,
        };
    public:
        HttpContext(const muduo::net::TcpConnectionPtr &conn, HttpServerPtr httpServerPtr)
        :httpRequest_(conn,httpServerPtr)
        {

        }

    
        bool ParseRequest(muduo::net::Buffer* buffer,muduo::Timestamp timestamp);
        bool isAll() { return state_ == kGotAll; }
        HttpRequest& getHttpRequest() { return httpRequest_; }
        void print();
        void clear();
    private:
        bool parseRequestLine(const std::string& requestLine);
        
        bool setMethod(const std::string& method);
        bool setPath(const std::string& path);
        bool setVersion(const std::string& version);
        bool setHeader(const std::string& header);
    private:
        HttpRequestParseState state_ = kExpectRequestLine;
        HttpRequest httpRequest_;
    };

}