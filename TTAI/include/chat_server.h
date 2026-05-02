#pragma once
#include "http_server.h"
#include "database_service.h"
#include "ai_factory.h"
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace chat
{

    struct ChatRecord
    {
        using json = nlohmann::json;
        http::HttpServerPtr httpServerPtr_;
        const muduo::net::TcpConnectionPtr conn_;
        int chatId_ = -1;
        std::string buffer_;
        std::string currentMessage_;
        json body_;
        std::shared_ptr<AIStrategy> strategy_;
        size_t total_tokens = 0;
        size_t output_tokens = 0;
        size_t input_tokens = 0;
        ChatRecord(http::HttpServerPtr httpServerPtr, const muduo::net::TcpConnectionPtr &conn)
            : httpServerPtr_(httpServerPtr), conn_(conn)
        {
        }
        ChatRecord(std::shared_ptr<ChatRecord> cr,
                   http::HttpServerPtr httpServerPtr,
                   const muduo::net::TcpConnectionPtr &conn)
            : httpServerPtr_(httpServerPtr),
              conn_(conn)
        {
            std::swap(chatId_, cr->chatId_);
            buffer_.swap(cr->buffer_);
            currentMessage_.swap(cr->currentMessage_);
            body_.swap(cr->body_);
            strategy_.swap(cr->strategy_);

            std::swap(total_tokens, cr->total_tokens);
            std::swap(output_tokens, cr->output_tokens);
            std::swap(input_tokens, cr->input_tokens);
        }
    };
    class ChatServer
    {
        bool IsValid(const std::string &str); // can create new file to write todo
        void AIHomePage(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIChatPage(const http::HttpRequest &req, http::HttpResponse *resp);
        void AILogin(const http::HttpRequest &req, http::HttpResponse *resp);
        void AILogout(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIRegister(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIProcessMessage(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIGetTitle(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIGetHistory(const http::HttpRequest &req, http::HttpResponse *resp);
        void AIGetAllTitle(const http::HttpRequest &req, http::HttpResponse *resp);
        void RegisterService()
        {
            server_->Get(std::regex("/chat"),
                         std::bind(&ChatServer::AIChatPage, this, std::placeholders::_1, std::placeholders::_2));
            server_->Get(std::regex("^/(.*)$"),
                         std::bind(&ChatServer::AIHomePage, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/login"),
                          std::bind(&ChatServer::AILogin, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/logout"),
                          std::bind(&ChatServer::AILogout, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/register"),
                          std::bind(&ChatServer::AIRegister, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/send"),
                          std::bind(&ChatServer::AIProcessMessage, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/history"),
                          std::bind(&ChatServer::AIGetHistory, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/title"),
                          std::bind(&ChatServer::AIGetTitle, this, std::placeholders::_1, std::placeholders::_2));
            server_->Post(std::string("/alltitle"),
                          std::bind(&ChatServer::AIGetAllTitle, this, std::placeholders::_1, std::placeholders::_2));
        }

    public:
        // ChatServer(const std::string &configFile);
        ChatServer(const nlohmann::json &config);
        std::string getLogPath() { return logPath_; }
        ChatServer() = delete;
        void flushTodisk()
        {
            userChatMutex_.lock();
            for (auto &e : userChat_)
            {
                auto userId=e.first;
                auto chatRecord=e.second;
                if (chatRecord->chatId_ != -1)
                {
                    // write to mysql
                    databaseService_->writeMessageFromJson(
                        userId, chatRecord->chatId_,
                        chatRecord->body_,
                        chat::StrategyFactory::getInstance().getStrategy("AliMultimodal"));
                }
            }
        }
        void start()
        {
            server_->start();
        }

    private:
        std::unique_ptr<http::HttpServer> server_;
        std::unique_ptr<DatabaseService> databaseService_;
        std::unordered_map<uint64_t, std::shared_ptr<ChatRecord>> userChat_; // mutex todo
        std::mutex userChatMutex_;
        std::string webHome_;
        std::string webChat_;
        std::string chatHistoryDir_;
        std::string logPath_;
    };
}