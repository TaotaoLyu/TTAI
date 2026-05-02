#include "chat_server.h"
#include "file_util.hpp"

namespace chat
{
    void ChatServer::AIChatPage(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        try
        {
            if (databaseService_->isSessionExist(req.getSessionId()) == false)
            {
                resp->version_ = "1.1", resp->status_ = "404", resp->describes_ = "cookie invalid";
                return;
            }

            std::string path = webChat_ + "/index.html";
            std::string fileContent;

            FileUtil::ReadFile(path, fileContent);

            resp->version_ = "1.1";
            resp->status_ = "200";
            resp->describes_ = "OK";

            resp->headers_["Content-Length"] = std::to_string(fileContent.size());

            resp->body_.swap(fileContent);
            resp->headers_["Content-Type"] = FileUtil::FileType(path);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "AIChatPage: " << e.what();
            resp->version_ = "1.1";
            resp->status_ = "404";
            resp->describes_ = "NOT FOUND";
            resp->headers_["Content-length"] = std::to_string(0);
        }
    }

}