#include "database_service.h"

namespace chat
{
    uint64_t DatabaseService::insertUser(const std::string &name, const std::string &password, const std::string &sessionId)
    {
        try
        {

            pool_.executeUpdate("INSERT INTO user (username, password, session_id, salt) VALUES (?, ?, ?, NULL)", 1, name, 2, password, 3, sessionId);
            auto ret = pool_.executeQuery("SELECT id FROM user WHERE username=?", 1, name);

            if (ret->next())
            {
                return ret->getUInt64("id");
            }
            return 0;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "insertUser error: " << e.what();
            throw;
        }
    }

    bool DatabaseService::isUserExist(const std::string &name)
    {

        auto ret = pool_.executeQuery("SELECT 1 FROM user WHERE username=? LIMIT 1", 1, name);
        return ret->next();
    }

    bool DatabaseService::isSessionExist(const std::string &sessionId)
    {

        auto ret = pool_.executeQuery("SELECT 1 FROM user WHERE session_id=? LIMIT 1", 1, sessionId);
        return ret->next();
    }

    DatabaseService::UserInfo DatabaseService::getUserInfoFromName(const std::string &name)
    {
        auto ret = pool_.executeQuery("SELECT id,password,session_id FROM user WHERE username=? LIMIT 1", 1, name);

        if (ret->next())
        {
            return {ret->getUInt64("id"), name, ret->getString("password"), ret->getString("session_id")};
        }
        return {};
    }

    DatabaseService::UserInfo DatabaseService::getUserInfoFromSession(const std::string &sessionId)
    {
        auto ret = pool_.executeQuery("SELECT id,username,password,session_id FROM user WHERE session_id=? LIMIT 1", 1, sessionId);

        if (ret->next())
        {
            return {ret->getUInt64("id"), ret->getString("username"), ret->getString("password"), ret->getString("session_id")};
        }
        return {};
    }

    uint64_t DatabaseService::getChatSessionId(uint64_t userId, uint64_t chatId)
    {

        pool_.executeUpdate(
            "INSERT INTO chat_sessions (user_id, chat_id) "
            "VALUES (?, ?) "
            "ON DUPLICATE KEY UPDATE id = id",
            1, userId,
            2, chatId);

        auto res = pool_.executeQuery(
            "SELECT id FROM chat_sessions "
            "WHERE user_id = ? AND chat_id = ? "
            "LIMIT 1",
            1, userId,
            2, chatId);

        if (res->next())
        {
            return res->getUInt64("id");
        }

        throw std::runtime_error("Failed to get chat session id");
    }
    void DatabaseService::insertMessage(uint64_t ChatSessionId, uint64_t index, std::string &role, std::string &message)
    {

        pool_.executeUpdate("INSERT INTO chat_messages (session_id, message_index, role, content) VALUES (?, ?, ?, ?)", 1, ChatSessionId, 2, index, 3, role, 4, message);
    }

    // SELECT message_id, session_id, message_index, role, content, created_at
    // FROM chat_messages
    // WHERE session_id = ?
    // ORDER BY message_index ASC;

    void DatabaseService::readMessageToJson(uint64_t userId, uint64_t chatId, nlohmann::json &body, std::shared_ptr<AIStrategy> strategy)
    {
        uint64_t ChatSessionId = getChatSessionId(userId, chatId);
        auto ret = pool_.executeQuery("SELECT role, content, created_at FROM chat_messages  WHERE session_id = ? ORDER BY message_index ASC", 1, ChatSessionId);
        while (ret->next())
        {
            strategy->addMessage(body, ret->getString("role"), ret->getString("content"));
        }
    }

    void DatabaseService::writeMessageFromJson(uint64_t userId, uint64_t chatId, nlohmann::json &body, std::shared_ptr<AIStrategy> strategy)
    {

        uint64_t ChatSessionId = getChatSessionId(userId, chatId);

        if (strategy->getMessageSize(body) <= 1)
            return;

        auto ret = pool_.executeQuery("SELECT message_count FROM chat_sessions WHERE id=?", 1, ChatSessionId);
        size_t cur = 0;
        if (ret->next())
        {
            cur = ret->getUInt64("message_count");
        }

        for (size_t i = cur + 1; i < strategy->getMessageSize(body); ++i)
        {
            std::string role;
            std::string message = strategy->getMessage(body, i, &role);
            insertMessage(ChatSessionId, i, role, message);
        }
        pool_.executeUpdate("UPDATE chat_sessions SET message_count = message_count + ? WHERE id = ?", 1, strategy->getMessageSize(body) - cur - 1, 2, ChatSessionId);
    }

    std::string DatabaseService::getTitle(uint64_t userId, uint64_t chatId)
    {
        uint64_t ChatSessionId = getChatSessionId(userId, chatId);
        auto ret = pool_.executeQuery("SELECT title FROM chat_sessions WHERE user_id = ? AND chat_id = ? LIMIT 1", 1, userId, 2, chatId);
        if (ret->next())
        {
            return ret->getString("title");
        }
        return "";
    }
    void DatabaseService::setTitle(uint64_t userId, uint64_t chatId, const std::string &title)
    {
        uint64_t ChatSessionId = getChatSessionId(userId, chatId);

        pool_.executeUpdate("UPDATE chat_sessions SET title=?  WHERE id = ?", 1, title, 2, ChatSessionId);
    }
    nlohmann::json DatabaseService::getAllTitle(uint64_t userId)
    {
        auto result = pool_.executeQuery("SELECT chat_id,title,created_at FROM chat_sessions WHERE user_id=? ORDER BY chat_id ASC", 1, userId);
        nlohmann::json ret;
        int cur = 0;
        while (result->next())
        {
            ret["chat"][cur]["chat_id"] = result->getUInt64("chat_id");
            ret["chat"][cur]["title"] = result->getString("title");
            ret["chat"][cur]["created_time"] = result->getString("created_at");
            cur++;
        }
        return ret;
    }
    std::string DatabaseService::getCreatedTime(uint64_t userId, uint64_t chatId)
    {
        uint64_t ChatSessionId = getChatSessionId(userId, chatId);
        auto result = pool_.executeQuery("SELECT created_at FROM chat_sessions WHERE id=? ORDER BY chat_id ASC", 1, ChatSessionId);
        if (result->next())
            return result->getString("created_at");
        return "";
    }

}