#pragma once
#include "db_connection_pool.h"
#include "ai_strategy.h"

namespace chat
{

    class DatabaseService
    {
    public:
        struct UserInfo
        {
            uint64_t id_;
            std::string name_;
            std::string password_;
            std::string sessionId_;
        };
        DatabaseService(const std::string &host,
                        const std::string &user,
                        const std::string &password,
                        const std::string &database,
                        uint32_t connNum)
            : pool_(db::DbConnectionPool::getInstance())
        {
            pool_.init(host, user, password, database, connNum);
        }

        DatabaseService() = delete;
        // userexist?
        // get session?
        // get id?

        uint64_t insertUser(const std::string &name, const std::string &password, const std::string &sessionId);
        bool isUserExist(const std::string &name);
        bool isSessionExist(const std::string &sessionId);
        UserInfo getUserInfoFromName(const std::string &name);
        UserInfo getUserInfoFromSession(const std::string &sessionId);
        void insertMessage(uint64_t ChatSessionId, uint64_t index, std::string &role, std::string &message);
        void readMessageToJson(uint64_t userId, uint64_t chatId, nlohmann::json &body, std::shared_ptr<AIStrategy> strategy);
        void writeMessageFromJson(uint64_t userId, uint64_t chatId, nlohmann::json &body, std::shared_ptr<AIStrategy> strategy);
        std::string getTitle(uint64_t userId, uint64_t chatId);
        void setTitle(uint64_t userId, uint64_t chatId,const std::string& title);
        std::string getCreatedTime(uint64_t userId, uint64_t chatId);
        nlohmann::json getAllTitle(uint64_t userId);


    private:
        uint64_t getChatSessionId(uint64_t userId, uint64_t chatId);

        db::DbConnectionPool &pool_;
    };

}

// CREATE TABLE user (
//     id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
//     username VARCHAR(255) NOT NULL COMMENT '用户名',
//     password VARCHAR(255) NOT NULL COMMENT '密码哈希',
//     session_id VARCHAR(255) NOT NULL COMMENT '当前登录会话ID',
//     salt VARCHAR(255) DEFAULT NULL COMMENT '盐值，可为空',

//     PRIMARY KEY (id),
//     UNIQUE KEY uk_username (username),
//     KEY idx_session_id (session_id)
// ) ENGINE=InnoDB
// DEFAULT CHARSET=utf8mb4
// COLLATE=utf8mb4_unicode_ci
// COMMENT='用户表';