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

namespace http
{
    const char *BadResponseLine = "HTTP/1.1 400 Bad Request\r\n\r\n";
    using HttpCallback = std::function<void(HttpRequest &, HttpResponse *)>;
    class HttpServer
    {
    public:
        HttpServer(uint16_t port,
                   std::string name,
                   HttpCallback httpCallback = nullptr,
                   uint32_t threadNum = 5,
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
                    tcpConnToSslConn_[conn] = std::make_unique<ssl::SslConection>(sslContext_);
                }

                conn->setContext(HttpContext(conn, this));
            }
            else
            {
                if (tcpConnToSslConn_.count(conn))
                    tcpConnToSslConn_.erase(conn);
            }
        }
        void OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp timeStamp)
        {
            // std::string message = buffer->retrieveAllAsString();
            // std::cout << message;
            // std::cout.flush();

            HttpContext *httpContext = boost::any_cast<HttpContext>(conn->getMutableContext());
            if (useSsl_ && !tcpConnToSslConn_[conn]->iskEstablished())
            {
                // handshake
                tcpConnToSslConn_[conn]->handShake(buffer);
                conn->send(tcpConnToSslConn_[conn]->getWriteBuffer());
            }
            while (true)
            {
                if (useSsl_)
                {
                    buffer = tcpConnToSslConn_[conn]->decrypt(buffer);
                }
                if (httpContext->ParseRequest(buffer, timeStamp) == false)
                {
                    send(conn, BadResponseLine);
                    conn->shutdown();
                }
                if (httpContext->isAll())
                {
                    // process request=>reponse and send
                    httpContext->print(); // debug
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
                    if (useSsl_ && tcpConnToSslConn_[conn]->isDecryptionComplete() == false)
                        continue;
                    break;
                }
            }
        }
        void send(const muduo::net::TcpConnectionPtr &conn, const std::string &message)
        {
            if (useSsl_)
                conn->send(tcpConnToSslConn_[conn]->encrypt(message));
            else
                conn->send(message);
        }
        void onRequest(HttpRequest &httpRequest)
        {

            HttpResponse httpResponse;
            handleRequest(httpRequest,&httpResponse);
            send(httpRequest.conn_,httpResponse);
            

            // if short connection throw exception
        }
        void handleRequest(const HttpRequest &req, HttpResponse *resp)
        {

            // router to do
        }

    private:
        std::string name_;
        uint32_t threadNum_;
        uint16_t port_;
        muduo::net::TcpServer server_;
        muduo::net::EventLoop eventLoop_;
        bool useSsl_;
        ssl::SslContext sslContext_;
        std::unordered_map<muduo::net::TcpConnectionPtr, ssl::SslConectionPtr> tcpConnToSslConn_;
        std::function<void(HttpRequest &, HttpResponse *)> httpCallback_;
    };

}