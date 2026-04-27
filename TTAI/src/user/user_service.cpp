#include "user_service.h"

namespace chat
{
    uint64_t UserService::insertUser(const std::string &name, const std::string &password, const std::string &sessionId)
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

    bool UserService::isUserExist(const std::string &name)
    {

        auto ret = pool_.executeQuery("SELECT 1 FROM user WHERE username=? LIMIT 1", 1, name);
        return ret->next();
    }

    bool UserService::isSessionExist(const std::string &sessionId)
    {

        auto ret = pool_.executeQuery("SELECT 1 FROM user WHERE session_id=? LIMIT 1", 1, sessionId);
        return ret->next();
    }

    UserService::UserInfo UserService::getUserInfo(const std::string &name)
    {
        auto ret=pool_.executeQuery("SELECT id,password,session_id FROM user WHERE username=? LIMIT 1",1,name);

        if(ret->next())
        {
            return {ret->getUInt64("id"),name,ret->getString("password"),ret->getString("session_id")};

        }
        return {};
        // if(ret->next())
        //     return ret->getUInt64("id");
        

        // return 0;

    }

}