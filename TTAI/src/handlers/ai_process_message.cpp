#include "chat_server.h"
#include "http_response.h"
#include <curl/curl.h>

namespace chat
{

    static size_t WriteBack(void *contents, size_t size, size_t nmeb, void *userp)
    {
        // printf("writeback\n");

        size_t totalSize = size * nmeb;
        ChatRecord *chatRecord = static_cast<ChatRecord *>(userp);
        std::string &buffer = chatRecord->buffer_;
        buffer.append(static_cast<char *>(contents), totalSize);

        // printf("=============================\n");
        // std::cout << buffer << std::endl;
        // printf("=============================\n");

        size_t pos = 0;
        while ((pos = buffer.find("\n\n")) != std::string::npos)
        {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 2);
            size_t pp = line.find("data:");
            if (pp == std::string::npos)
            {
                continue;
            }
            std::string data = line.substr(pp + 5);
            nlohmann::json res = nlohmann::json::parse(data);

            std::string message = chatRecord->strategy_->getMessage(res);
            chatRecord->currentMessage_ += message;

            // send message todo

            // const ChatRecord * p=chatRecord;
            // printf("send\n");
            std::string sendMessage = "data:";
            nlohmann::json sendJson;
            sendJson["content"] = message;
            sendMessage += sendJson.dump();
            sendMessage += "\n\n";
            // std::cout << sendMessage << std::endl;
            chatRecord->httpServerPtr_->send(chatRecord->conn_, sendMessage);
            // printf("send\n");
        }

        return totalSize;
    }
    static void prepareAIChatRequest(CURL *curl, curl_slist *&headers, std::shared_ptr<ChatRecord> chatRecord)
    {
        std::string authHeader = "Authorization: Bearer " + chatRecord->strategy_->getAPI();
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "X-DashScope-SSE: enable");
        curl_easy_setopt(curl, CURLOPT_URL, chatRecord->strategy_->getURL().c_str());
        // std::cout << "url: " << chatRecord->strategy_->getURL() << std::endl;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBack);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, chatRecord.get());
    }
    static void sendSSEStartResponse(const http::HttpRequest &req)
    {
        http::HttpResponse response;
        response.version_ = "1.1";
        response.status_ = "200";
        response.describes_ = "OK";

        response.headers_["Content-Type"] = "text/event-stream";
        response.headers_["Cache-Control"] = "no-cache";
        response.headers_["Connection"] = "keep-alive";

        auto it = req.headers_.find("Origin");
        if (it != req.headers_.end())
        {
            response.headers_["Access-Control-Allow-Origin"] = it->second;
        }

        response.headers_["Access-Control-Allow-Credentials"] = "true";

        req.httpServerPtr_->send(req.conn_, response);
    }

    static int CurlDebugCallback(CURL *handle,
                                 curl_infotype type,
                                 char *data,
                                 size_t size,
                                 void *userptr)
    {
        std::string text(data, size);

        switch (type)
        {
        case CURLINFO_TEXT:
            std::cerr << "== Info: " << text;
            break;

        case CURLINFO_HEADER_OUT:
            std::cerr << "\n=> Send header:\n";
            std::cerr << text;
            break;

        case CURLINFO_DATA_OUT:
            std::cerr << "\n=> Send data:\n";
            std::cerr << text << "\n";
            break;

        case CURLINFO_HEADER_IN:
            std::cerr << "\n<= Recv header:\n";
            std::cerr << text;
            break;

        case CURLINFO_DATA_IN:
            std::cerr << "\n<= Recv data:\n";
            std::cerr << text << "\n";
            break;

        default:
            break;
        }

        return 0;
    }
    void ChatServer::AIProcessMessage(const http::HttpRequest &req, http::HttpResponse *resp)
    {

        // load user session
        // "user_id: 2"
        // {"chat_id":1,"message":"hello"};
        nlohmann::json userBody = nlohmann::json::parse(req.body_);
        if (databaseService_->isSessionExist(req.getSessionId()) == false)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
            return;
        }
        uint64_t userId = databaseService_->getUserInfoFromSession(req.getSessionId()).id_;
        int chatId = userBody.value("chat_id", -1);
        std::string userMessage = userBody.value("message", "");
        std::shared_ptr<ChatRecord> chatRecord;
        {
            std::lock_guard<std::mutex> lock(userChatMutex_);
            auto userChatIt = userChat_.find(userId);

            // auto chatRecord = (*userChatIt).second;

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
        // printf("start process=====================\n");

        CURL *curl = curl_easy_init();
        struct curl_slist *headers = nullptr;

        prepareAIChatRequest(curl, headers, chatRecord);
        chatRecord->strategy_->addMessage(chatRecord->body_, "user", userMessage);
        std::string bodyStr = chatRecord->body_.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());

        chatRecord->buffer_.clear();
        chatRecord->currentMessage_.clear();

        // send http ok tell client to start sse
        sendSSEStartResponse(req);

        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);
        CURLcode ret = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        // free resources
        if (ret == CURLE_OK)
        {

            chatRecord->strategy_->addMessage(chatRecord->body_, "assistant", chatRecord->currentMessage_);
        }
        else
        {
            throw std::runtime_error("curl error");
        }
        req.httpServerPtr_->send(req.conn_, "data:[DONE]\n\n");
        throw std::runtime_error("normal close(SSE END)");
    }

}