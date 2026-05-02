#include "chat_server.h"
#include <curl/curl.h>

namespace chat
{

    static size_t WriteBack(void *contents, size_t size, size_t nmeb, void *buffer)
    {
        size_t totalSize = size * nmeb;
        std::string *response = static_cast<std::string *>(buffer);
        response->append(static_cast<char *>(contents), totalSize);

        // printf("=============================\n");
        // std::cout << (*response) << std::endl;
        // printf("=============================\n");

        return totalSize;
    }

    static std::string curlGetTitle(const std::string &message)
    {

        CURL *curl = curl_easy_init();
        struct curl_slist *headers = nullptr;
        auto strategy = chat::StrategyFactory::getInstance().getStrategy("AliMultimodal");
        std::string authHeader = "Authorization: Bearer " + strategy->getAPI();
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, strategy->getURL().c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        nlohmann::json body = strategy->initBody();
        strategy->addMessage(body, "user", message);
        strategy->addMessage(body, "user", "你是聊天标题生成器。根据用户的第一条消息生成一个简短标题。要求：不超过10个字，以中文为主，可以保留必要的英文缩写、技术名词和符号，例如 C++、SSE、HTTP、MySQL。只输出标题本身，不要解释，不要引号，不要句号，概括核心主题。只输出聊天标题。");
        std::string bodyStr = body.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
        // std::cout<<"body: \n"<<bodyStr<<std::endl;

        std::string buffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBack);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        CURLcode ret = curl_easy_perform(curl);
        // std::cout<<"buffer: "<<buffer<<std::endl;
        nlohmann::json titleJson = nlohmann::json::parse(buffer);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return strategy->getMessage(titleJson);
    }

    void ChatServer::AIGetTitle(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        nlohmann::json userBody = nlohmann::json::parse(req.body_);
        if (databaseService_->isSessionExist(req.getSessionId()) == false)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
            return;
        }

        // {"chat_id":2,“message”:"....."}
        // {"title":"...","created_time":""}
        // created_time
        uint64_t userId = databaseService_->getUserInfoFromSession(req.getSessionId()).id_;
        int chatId = userBody.value("chat_id", -1);
        std::string userMessage = userBody.value("message", "");
        if (userMessage.size() > 100)
        {
            userMessage.resize(100);
        }
        if (userMessage.size() == 0)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "message empty";
            return;
        }
        std::string title = databaseService_->getTitle(userId, chatId);
        if (title == "" || title == "New Chat")
        {
            title = curlGetTitle(userMessage);
            // std::cout<<"title: "<<title<<std::endl;
            while (title.size() > 30)
            {
                title = curlGetTitle(userMessage);
            }
            databaseService_->setTitle(userId, chatId, title);
        }

        nlohmann::json returnBody;
        returnBody["title"] = title;
        returnBody["created_time"] = databaseService_->getCreatedTime(userId,chatId);

        resp->version_ = "1.1", resp->status_ = "200", resp->describes_ = "OK";
        resp->headers_["Content-type"] = "application/json";

        resp->body_ = returnBody.dump();
    }
}