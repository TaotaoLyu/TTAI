#include"http_server.h"
#include<iostream>
#include<memory>
#include"ssl_config.h"

int main()
{
    // http::HttpServer* httpserver=
    ssl::SslConfig sslConfig("/home/ltt/TTAI/http_server/test/server.crt","/home/ltt/TTAI/http_server/test/server.key");
    // std::unique_ptr<http::HttpServer> httpserver=std::make_unique<http::HttpServer>(8000,"test_server");
    std::unique_ptr<http::HttpServer> httpserver=std::make_unique<http::HttpServer>(8000,"test_server",true,sslConfig);
    
    
    httpserver->start();
    return 0;
}