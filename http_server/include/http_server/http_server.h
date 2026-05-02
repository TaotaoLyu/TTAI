#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <string>
#include <functional>
#include <muduo/base/Logging.h>
#include <iostream>
#include "http_context.h"
#include "ssl_connection.h"
#include "http_response.h"
#include "router.h"
#include <mutex>

namespace http
{
    using HttpCallback = std::function<void(HttpRequest &, HttpResponse *)>;
    class HttpServer
    {
    public:
        HttpServer(uint16_t port,
                   std::string name,
                   uint32_t threadNum = 5,
                   HttpCallback httpCallback = nullptr,
                   muduo::net::TcpServer::Option option = muduo::net::TcpServer::kReusePort)
            : name_(name),
              httpCallback_(std::bind(&HttpServer::handleRequest, this, std::placeholders::_1, std::placeholders::_2)),
              threadNum_(threadNum),
              port_(port),
              server_(&eventLoop_, muduo::net::InetAddress(port), name, option)
        {
            setThreadNum(threadNum_);
            server_.setConnectionCallback(
                std::bind(&HttpServer::OnConnection, this, std::placeholders::_1));
            server_.setMessageCallback(
                std::bind(&HttpServer::OnMessage, this,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3));
        }
        HttpServer(uint16_t port,
                   std::string name,
                   bool useSsl,
                   const ssl::SslConfig &sslConfig,
                   uint32_t threadNum = 5,
                   muduo::net::TcpServer::Option option = muduo::net::TcpServer::kReusePort)
            : name_(name),
              threadNum_(threadNum),
              useSsl_(useSsl),
              sslContext_(sslConfig),
              port_(port),
              server_(&eventLoop_, muduo::net::InetAddress(port), name, option)
        {
            setThreadNum(threadNum_);
            server_.setConnectionCallback(
                std::bind(&HttpServer::OnConnection, this, std::placeholders::_1));
            server_.setMessageCallback(
                std::bind(&HttpServer::OnMessage, this,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          std::placeholders::_3));
        }

        void setThreadNum(int num)
        {
            server_.setThreadNum(num);
        }

        void start()
        {
            server_.start();
            eventLoop_.loop();
        }
        void OnConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                if (useSsl_)
                {
                    std::lock_guard<std::mutex> lock(sslConnMutex_);
                    tcpConnToSslConn_[conn] = std::make_unique<ssl::SslConection>(sslContext_);
                }

                conn->setContext(HttpContext(conn, this));
            }
            else
            {
                if (useSsl_)
                {
                    std::lock_guard<std::mutex> lock(sslConnMutex_);
                    if (tcpConnToSslConn_.count(conn))
                        tcpConnToSslConn_.erase(conn);
                }
            }
        }
        void OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp timeStamp)
        {
            // std::string message = buffer->retrieveAllAsString();
            // std::cout << message;
            // std::cout.flush();
            const char *BadResponseLine = "HTTP/1.1 404 Bad Request\r\n\r\n";

            try
            {
                HttpContext *httpContext = boost::any_cast<HttpContext>(conn->getMutableContext());
                ssl::SslConectionPtr sslPtr = nullptr;
                if (useSsl_)
                {
                    std::lock_guard<std::mutex> lock(sslConnMutex_);
                    sslPtr = tcpConnToSslConn_[conn];
                }
                if (useSsl_ && !sslPtr->iskEstablished())
                {
                    // handshake
                    sslPtr->handShake(buffer);
                    conn->send(sslPtr->getWriteBuffer());
                }

                while (true)
                {
                    if (useSsl_)
                    {
                        std::lock_guard<std::mutex> lock(sslConnMutex_);

                        buffer = sslPtr->decrypt(buffer);
                    }
                    if (httpContext->ParseRequest(buffer, timeStamp) == false)
                    {
                        send(conn, BadResponseLine);
                        conn->shutdown();
                    }
                    if (httpContext->isAll())
                    {
                        // process request=>reponse and send
                        // httpContext->print(); // debug
                        onRequest(httpContext->getHttpRequest());
                        httpContext->clear();
                        // send(conn,"HTTP/1.1 200 OK\r\nContent-Length:20\r\n\r\n{\"name\":\"taotaoLyu\"}");

                        // debug
                        // if (useSsl_)
                        // {
                        //     tcpConnToSslConn_[conn]->shutdown();
                        //     conn->send(tcpConnToSslConn_[conn]->getWriteBuffer());
                        // }
                        // conn->shutdown();
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock(sslConnMutex_);

                        if (useSsl_ && sslPtr->isDecryptionComplete() == false)
                            continue;
                        break;
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::string exceptMessage(e.what());
                if (exceptMessage.find("normal"))
                    LOG_INFO << e.what();
                else
                    LOG_ERROR << e.what();
                // send(conn, BadResponseLine);
                if (useSsl_)
                {
                    ssl::SslConectionPtr sslPtr = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(sslConnMutex_);
                        sslPtr = tcpConnToSslConn_[conn];
                    }
                    sslPtr->shutdown();
                    conn->send(sslPtr->getWriteBuffer());
                }
                conn->shutdown();
            }
        }
        void send(const muduo::net::TcpConnectionPtr &conn, const std::string &message)
        {

            if (useSsl_)
            {
                ssl::SslConectionPtr sslPtr = nullptr;
                {
                    std::lock_guard<std::mutex> lock(sslConnMutex_);
                    sslPtr = tcpConnToSslConn_[conn];
                }
                conn->send(sslPtr->encrypt(message));
            }
            else
                conn->send(message);
        }
        void onRequest(HttpRequest &httpRequest)
        {

            HttpResponse httpResponse;
            handleRequest(httpRequest, &httpResponse);
            // std::cout<<std::string(httpResponse)<<std::endl;
            send(httpRequest.conn_, httpResponse);

            // if short connection throw exception

            if (httpRequest.headers_.count("Connection") && httpRequest.headers_["Connection"] == "close")
            {
                throw std::runtime_error("client want to close connection");
            }
        }
        void Get(const std::string &path, router::Router::HandlerCallback handlerCallback)
        {
            router_.registerHandler(HttpRequest::kGet, path, handlerCallback);
        }
        void Get(const std::regex &reg, router::Router::HandlerCallback handlerCallback)
        {
            router_.registerHandler(HttpRequest::kGet, reg, handlerCallback);
        }
        void Post(const std::string &path, router::Router::HandlerCallback handlerCallback)
        {
            router_.registerHandler(HttpRequest::kPost, path, handlerCallback);
        }
        void Post(const std::regex &reg, router::Router::HandlerCallback handlerCallback)
        {
            router_.registerHandler(HttpRequest::kPost, reg, handlerCallback);
        }
        void handleRequest(const HttpRequest &req, HttpResponse *resp)
        {

            // cors
            // std::cout << "debug " << req.method_ << " " << HttpRequest::kOptions << std::endl;
            if (req.method_ == HttpRequest::kOptions)
            {
                // printf("hello=============================\n");
                resp->version_ = "1.1";
                resp->status_ = "204";
                resp->describes_ = "No Content";

                auto it = req.headers_.find("Origin");
                if (it != req.headers_.end())
                    resp->headers_["Access-Control-Allow-Origin"] = it->second;

                resp->headers_["Access-Control-Allow-Credentials"] = "true";
                resp->headers_["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
                // resp->headers_["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
                resp->headers_["Access-Control-Allow-Headers"] = "Content-Type";
                resp->headers_["Access-Control-Max-Age"] = "86400";
                resp->headers_["Content-Length"] = "0";

                return;
            }

            // router to do

            if (router_.route(req, resp) == false)
            {
                throw std::runtime_error("not found route");
            }

            auto it = req.headers_.find("Origin");
            if (it != req.headers_.end())
                resp->headers_["Access-Control-Allow-Origin"] = it->second;

            resp->headers_["Access-Control-Allow-Credentials"] = "true";
        }

    private:
        std::string name_;
        uint32_t threadNum_;
        uint16_t port_;
        muduo::net::EventLoop eventLoop_;

        muduo::net::TcpServer server_;
        bool useSsl_;
        ssl::SslContext sslContext_;
        std::unordered_map<muduo::net::TcpConnectionPtr, ssl::SslConectionPtr> tcpConnToSslConn_; // mutex
        std::mutex sslConnMutex_;
        std::function<void(HttpRequest &, HttpResponse *)> httpCallback_;
        router::Router router_;
    };

}