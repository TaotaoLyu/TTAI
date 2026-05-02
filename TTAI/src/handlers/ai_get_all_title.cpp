#include "chat_server.h"

namespace chat
{
    void ChatServer::AIGetAllTitle(const http::HttpRequest &req, http::HttpResponse *resp)
    {

        // nlohmann::json userBody = nlohmann::json::parse(req.body_);
        if (databaseService_->isSessionExist(req.getSessionId()) == false)
        {
            resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
            return;
        }
        uint64_t userId = databaseService_->getUserInfoFromSession(req.getSessionId()).id_;


        resp->version_="1.1",resp->status_="200",resp->describes_="OK";
        resp->headers_["Content-type"]="application/json";

        resp->body_=databaseService_->getAllTitle(userId).dump();
    }

}