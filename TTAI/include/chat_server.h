#pragma once
#include "http_server.h"
#include "user_service.h"
#include <nlohmann/json.hpp>

namespace chat
{
    class ChatServer
    {
        bool IsValid(const std::string &str);
        void AIHomePage(const http::HttpRequest &req, http::HttpResponse *resp);
        void AILogin(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIRegister(const http::HttpRequest &req, http::HttpResponse *resp);

    public:
        ChatServer(const std::string &configFile);
        void RegisterService()
        {
            server_->Get(std::regex("^/(.*)$"),
                         std::bind(&ChatServer::AIHomePage, this, std::placeholders::_1, std::placeholders::_2));
            server_->Get(std::string("/login"),
                         std::bind(&ChatServer::AILogin, this, std::placeholders::_1, std::placeholders::_2));
            server_->Get(std::string("/register"),
                         std::bind(&ChatServer::AIRegister, this, std::placeholders::_1, std::placeholders::_2));
        }
        ChatServer() = delete;
        void start()
        {
            server_->start();
        }

    public:
        std::unique_ptr<http::HttpServer> server_;
        std::unique_ptr<UserService> userService_;
        std::string webHome_;
        std::string webChat_;
        std::string chatHistoryDir_;
    };

}