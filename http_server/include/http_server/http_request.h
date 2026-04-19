#pragma once
#include <string>
#include <stdint.h>
#include <unordered_map>
#include <muduo/net/TcpServer.h>

namespace http
{
    class HttpServer;
    using HttpServerPtr = HttpServer *;
    class HttpRequest
    {
    public:
        enum Method
        {
            kInvalid,
            kGet,
            kPost,
            kHead,
            kPut,
            kDelete,
            kOptions,
        };
        HttpRequest(const muduo::net::TcpConnectionPtr &conn, HttpServerPtr httpServerPtr)
            : conn_(conn),
              httpServerPtr_(httpServerPtr)
        {
            if(!conn_||!httpServerPtr_)
                throw std::runtime_error("create HttpRequest failed");
        }
        void swap(HttpRequest *httpRequest)
        {
            std::swap(method_, httpRequest->method_);
            path_.swap(httpRequest->path_);
            version_.swap(httpRequest->version_);
            // path_parameters_.swap(httpRequest->path_parameters_);
            queryParameters_.swap(httpRequest->queryParameters_);
            headers_.swap(httpRequest->headers_);
            std::swap(contentLength_, httpRequest->contentLength_);
            body_.swap(httpRequest->body_);
            std::swap(receive_time_, httpRequest->receive_time_);
        }
        Method method_ = kInvalid;
        std::string path_;
        std::string version_ = "Unknown";
        // std::unordered_map<std::string, std::string> path_parameters_;
        std::unordered_map<std::string, std::string> queryParameters_;
        std::unordered_map<std::string, std::string> headers_;
        uint64_t contentLength_ = 0;
        std::string body_;
        muduo::Timestamp receive_time_;
        HttpServerPtr httpServerPtr_;
        const muduo::net::TcpConnectionPtr conn_;
    };
}