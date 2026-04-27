#include "chat_server.h"

namespace chat
{
    // { "user":"taotao", "password":"123456"}
    // {"status":0/1/2}
    // 0 -> success
    // 1 -> not found user
    // 2 -> password error
    // 3 -> formation error
    // 4 -> unKnow error
    void ChatServer::AILogin(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        using json = nlohmann::json;
        json returnBody;

        try
        {
            json body = json::parse(req.body_);

            std::string user = body.value("user", "");

            std::string password = body.value("password", "");

            if (IsValid(user) && IsValid(password))
            {
                if (userService_->isUserExist(user))
                {
                    // returnBody
                    auto ret = userService_->getUserInfo(user);
                    if (password == ret.password_)
                    {
                        std::string cookie = "sessionId=" + ret.sessionId_;
                        cookie += "; Expires=Fri, 01 Jan 2038 00:00:00 GMT; Path=/; Secure; HttpOnly";
                        returnBody["status"] = 0;
                        resp->headers_["Set-Cookie"] = std::move(cookie);
                    }
                    else
                    {
                        returnBody["status"] = 2;
                    }
                }
                else
                {
                    returnBody["status"] = 1;
                }
            }
            else
            {
                returnBody["status"] = 3;
            }
        }
        catch (...)
        {
            returnBody["status"] = 4;
        }

        resp->version_ = "1.1";
        resp->status_ = "200";
        resp->describes_ = "OK";
        resp->body_ = returnBody.dump();
        // Content-Type: application/json
        resp->headers_["Content-Type"] = "application/json";
    }

}