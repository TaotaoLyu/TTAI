#include "chat_server.h"
#include <fstream>
#include "ssl_config.h"
#include <nlohmann/json.hpp>

namespace chat
{
    // ChatServer::ChatServer(const std::string &configFile)
    ChatServer::ChatServer(const nlohmann::json &config)
        : server_(nullptr),
          databaseService_(nullptr)
    {

        try
        {
            // std::ifstream fin(configFile);
            // using json = nlohmann::json;
            // json config;
            // fin >> config;
            bool sslEnable = config["ssl"].value("enable", true);

            if (sslEnable)
            {
                uint32_t port = config["server"].value("port", 443);
                uint32_t threadNum = config["server"].value("thread_num", 5);
                std::string certFile = config["ssl"].value("cert_file", "");
                std::string keyFile = config["ssl"].value("key_file", "");
                ssl::SslConfig sslc(certFile, keyFile);
                server_ = std::make_unique<http::HttpServer>(port, "chatServer", true, sslc, threadNum);
            }
            else
            {
                uint32_t port = config["server"].value("port", 80);
                uint32_t threadNum = config["server"].value("thread_num", 5);
                server_ = std::make_unique<http::HttpServer>(port, "chatServer", threadNum);
            }
            webHome_ = config["web"].value("home", "");
            webChat_ = config["web"].value("chat", "");
            chatHistoryDir_ = config["chat"].value("history_dir", "");
            LOG_INFO << "init server success";

            // init mysql

            std::string mysqlHost = config["mysql"].value("host", "127.0.0.1");
            uint32_t mysqlPort = config["mysql"].value("port", 3306);
            std::string user = config["mysql"].value("user", "");
            std::string password = config["mysql"].value("password", "");
            std::string database = config["mysql"].value("database", "");
            uint32_t poolSize = config["mysql"].value("pool_size", 5);
            databaseService_ = std::make_unique<DatabaseService>(mysqlHost + ":" + std::to_string(mysqlPort), user, password, database, poolSize);

            // fin.close();
            LOG_INFO << "init MySql success";

            RegisterService();
            LOG_INFO << "RegisterService success";

            std::string apiKey=config["model"].value("api","");
            std::string modelName=config["model"].value("name","");
            std::string url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation";

            StrategyRegister<AliMultimodal>("AliMultimodal", modelName, apiKey, url);

            LOG_INFO << "ai config loading success";

            logPath_=config["log"].value("file","/dev/null");

        }
        catch (const std::exception &e)
        {
            LOG_FATAL << "init error: " << e.what();
            exit(1);
        }
    }

    bool ChatServer::IsValid(const std::string &str)
    {
        if (str.empty())
        {
            return false;
        }
        for (unsigned char c : str)
        {
            if (!std::isalnum(c))
            {
                return false;
            }
        }
        return true;
    }

}