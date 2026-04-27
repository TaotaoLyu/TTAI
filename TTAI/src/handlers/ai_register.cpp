#include "chat_server.h"
#include <random>

namespace chat
{

    static std::string generateSessionId(size_t length = 10)
    {
        const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_int_distribution<> dis(0, chars.size() - 1);

        std::string sessionId;
        sessionId.reserve(length);
        for (size_t i = 0; i < length; ++i)
        {
            sessionId += chars[dis(gen)];
        }
        return sessionId;
    }
    

    // { "user":"taotao", "password":"123456"}
    // {"status":0/1/2}
    // 0 -> success
    // 1 -> user exist
    // 2 -> formation error
    // 3 -> unKnow error
    void ChatServer::AIRegister(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        //
        using json = nlohmann::json;
        json ret;

        try
        {
            json body = json::parse(req.body_);
            std::string name = body.value("user", "");
            std::string password = body.value("password", "");

            if (IsValid(name) && IsValid(password))
            {

                if (userService_->isUserExist(name))
                {
                    ret["status"] = 1;
                }
                else
                {
                    std::string sessionId;
                    while (true)
                    {
                        sessionId = generateSessionId(10);
                        if (userService_->isSessionExist(sessionId) == false)
                            break;
                    }
                    userService_->insertUser(name, password, sessionId);
                    std::string cookie = "sessionId=" + sessionId;
                    cookie += "; Expires=Fri, 01 Jan 2038 00:00:00 GMT; Path=/; Secure; HttpOnly";
                    resp->headers_["Set-Cookie"] = std::move(cookie);
                    ret["status"] = 0;
                }
            }
            else
            {
                ret["status"] = 2;
            }
        }
        catch (...)
        {

            ret["status"] = 3;
        }
        resp->version_ = "1.1";
        resp->status_ = "200";
        resp->describes_ = "OK";
        resp->body_ = ret.dump();
        // Content-Type: application/json
        resp->headers_["Content-Type"] = "application/json";
    }

}