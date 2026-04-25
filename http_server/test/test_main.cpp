#include "http_server.h"
#include <iostream>
#include <memory>
#include <fstream>
#include "ssl_config.h"

void IndexHtml(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // printf("11111111111111111111111111\n");
    resp->status_ = "200";
    resp->describes_ = "OK";
    std::string path = "/home/ltt/TTAI/http_server/test/dist" + req.path_;
    if (path[path.size() - 1] == '/')
    {
        path += "index.html";
    }
    std::ifstream fin(path, std::ios::binary);
    // std::ifstream fin("/home/ltt/TTAI/http_server/test/index.html",std::ios::binary);
    fin.seekg(0, std::ios::end);
    size_t fileSize = fin.tellg();
    fin.seekg(0, std::ios::beg);
    resp->body_.resize(fileSize);
    if (fileSize > 0)
    {
        fin.read(&resp->body_[0], fileSize);
    }
    if (path.find(".html") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "text/html";
    }
    else if (path.find(".css") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "text/css";
    }
    else if (path.find(".js") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "application/javascript";
    }
    else if (path.find(".png") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "image/png";
    }
    else if (path.find(".jpg") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "image/jpeg";
    }
    else if (path.find(".jpeg") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "image/jpeg";
    }
    else if (path.find(".json") != std::string::npos)
    {
        resp->headers_["Content-Type"] = "application/json";
    }
    else
    {
        resp->headers_["Content-Type"] = "text/plain";
    }

    // resp->headers_["Content-Type"]="text/html; charset=utf-8";
    // printf("333333333333333333333333333\n");
}

int main()
{
    // http::HttpServer* httpserver=
    ssl::SslConfig sslConfig("/home/ltt/TTAI/http_server/test/server.crt", "/home/ltt/TTAI/http_server/test/server.key");
    // std::unique_ptr<http::HttpServer> httpserver=std::make_unique<http::HttpServer>(8000,"test_server");
    std::unique_ptr<http::HttpServer> httpserver = std::make_unique<http::HttpServer>(8001, "test_server", true, sslConfig);

    httpserver->Get(std::regex("^/(.*)$"), IndexHtml);
    httpserver->start();
    return 0;
}