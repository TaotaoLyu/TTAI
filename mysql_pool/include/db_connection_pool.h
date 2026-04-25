#pragma once
#include <queue>
#include <memory>
#include <mutex>
#include<condition_variable>
#include<thread>
#include "db_connection.h"

namespace db
{
    class DbConnectionPool
    {
public:
        void init(const std::string &host,
                  const std::string &user,
                  const std::string &password,
                  const std::string &database,
                  uint32_t connNum);
        static DbConnectionPool &getInstance()
        {
            static DbConnectionPool instance;
            return instance;
        }
        template <class... Args>
        sql::ResultSet *executeQuery(const std::string &sql, Args &&...args)
        {
            try
            {

                std::shared_ptr<DbConnection> conn = getConnection();
                return conn->executeQuery(sql, std::forward<Args>(args)...);
            }
            catch (const sql::SQLException &e)
            {
                LOG_ERROR << "executeQuery error: " << e.what();
                throw;
            }
        }
        template <class... Args>
        sql::ResultSet *executeUpdate(const std::string &sql, Args &&...args)
        {
            try
            {

                std::shared_ptr<DbConnection> conn = getConnection();
                return conn->executeUpdate(sql, std::forward<Args>(args)...);
            }
            catch (const sql::SQLException &e)
            {
                LOG_ERROR << "executeUpdate error: " << e.what();
                throw;
            }
        }



    private:
        void check();

        std::shared_ptr<DbConnection> getConnection();
        std::shared_ptr<DbConnection> createConnection();
        std::string host_;
        std::string user_;
        std::string password_;
        std::string database_;
        uint32_t connNum_;
        std::queue<std::shared_ptr<DbConnection>> connections_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool initialized_ = false;
    };
} // namespace db
