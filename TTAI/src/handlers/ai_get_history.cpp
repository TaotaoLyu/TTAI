#include "chat_server.h"

namespace chat
{
    void ChatServer::AIGetHistory(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        nlohmann::json userBody = nlohmann::json::parse(req.body_);
        if (databaseService_->isSessionExist(req.getSessionId()) == false)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
            return;
        }

        uint64_t userId = databaseService_->getUserInfoFromSession(req.getSessionId()).id_;
        int chatId = userBody.value("chat_id", -1);

        std::shared_ptr<ChatRecord> chatRecord;
        {
            std::lock_guard<std::mutex> lock(userChatMutex_);
            auto userChatIt = userChat_.find(userId);
            // auto chatRecord = (*userChatIt).second;
            // get chatrecord function
            if (userChatIt == userChat_.end())
            {
                userChatIt = userChat_.insert(std::make_pair(userId, std::make_shared<ChatRecord>(req.httpServerPtr_, req.conn_))).first;
                chatRecord = (*userChatIt).second;
            }
            else
            {
                // update the ptr of connection
                chatRecord = (*userChatIt).second;
                auto p = std::make_shared<ChatRecord>(chatRecord, req.httpServerPtr_, req.conn_);
                userChat_[userId] = p;
                chatRecord = p;
            }
        }

        if (chatRecord->chatId_ != chatId)
        {

            if (chatRecord->chatId_ != -1)
            {
                // write to mysql
                databaseService_->writeMessageFromJson(
                    userId, chatRecord->chatId_,
                    chatRecord->body_,
                    chat::StrategyFactory::getInstance().getStrategy("AliMultimodal"));
            }

            chatRecord->chatId_ = chatId;
            chatRecord->strategy_ = chat::StrategyFactory::getInstance().getStrategy("AliMultimodal"); // to do
            chatRecord->body_ = chatRecord->strategy_->initBody();
            // lode data from mysql
            databaseService_->readMessageToJson(userId, chatRecord->chatId_, chatRecord->body_, chatRecord->strategy_);
        }

        nlohmann::json respBody;
        for (int i = 1; i < chatRecord->strategy_->getMessageSize(chatRecord->body_); ++i)
        {
            std::string role;
            respBody["message"][i - 1]["content"] = chatRecord->strategy_->getMessage(chatRecord->body_, i, &role);
            respBody["message"][i - 1]["role"] = role;
        }
        resp->version_ = "1.1", resp->status_ = "200", resp->describes_ = "OK";
        resp->headers_["Content-type"] = "application/json";
        resp->body_ = respBody.dump();
    }

}