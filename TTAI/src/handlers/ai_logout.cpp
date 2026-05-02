#include "chat_server.h"

namespace chat
{
    void ChatServer::AILogout(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        if (databaseService_->isSessionExist(req.getSessionId()) == false)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
            return;
        }
        std::string sessionId = req.getSessionId();

        nlohmann::json returnBody;
        std::string cookie = "sessionId=" + sessionId;
        cookie += "; Max-Age=0; Path=/; HttpOnly; SameSite=None; Secure";
        returnBody["status"] = 0;
        resp->headers_["Set-Cookie"] = std::move(cookie);
    }

}