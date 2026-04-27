#include "chat_server.h"
#include "file_util.hpp"

namespace chat
{
    void ChatServer::AIHomePage(const http::HttpRequest &req, http::HttpResponse *resp)
    {
        try
        {
            std::string path = webHome_ + req.path_;
            if (path[path.size() - 1] == '/')
            {
                path += "index.html";
            }

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
            LOG_ERROR<<"AIHomePage: "<<e.what();
             resp->version_ = "1.1";
            resp->status_ = "404";
            resp->describes_ = "NOT FOUND";
            resp->headers_["Content-length"] = std::to_string(0);
        }
    }
}