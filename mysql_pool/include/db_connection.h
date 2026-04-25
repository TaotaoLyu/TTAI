#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <muduo/base/Logging.h>
// #include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
// #include<mysql/mysql.h>

namespace db
{
    class DbConnection
    {
    public:
        DbConnection(const std::string &host,
                     const std::string &user,
                     const std::string &password,
                     const std::string &database);
        DbConnection() = delete;

        DbConnection &operator=(const DbConnection &) = delete;
        DbConnection(const DbConnection &) = delete;
        void reconnect();

        ~DbConnection();

        bool ping();
        template <class... Args>
        sql::ResultSet *executeQuery(const std::string &sql, Args &&...args)
        {

            // std::unique_ptr<sql::Statement> stem(conn_->createStatement());
            // return stem->executeQuery(sql);
            try
            {
                std::unique_ptr<sql::PreparedStatement> pstem(
                    conn_->prepareStatement(sql));
                bindParams(pstem.get(), std::forward<Args>(args)...);
                return pstem->executeQuery();
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

            // std::unique_ptr<sql::Statement> stem(conn_->createStatement());
            // return stem->executeQuery(sql);
            try
            {
                std::unique_ptr<sql::PreparedStatement> pstem(
                    conn_->prepareStatement(sql));
                bindParams(pstem.get(), std::forward<Args>(args)...);
                return pstem->executeUpdate();
            }
            catch (const sql::SQLException &e)
            {
                LOG_ERROR << "executeUpdate error: " << e.what();
                throw;
            }
        }

    private:
        void bindParams(sql::PreparedStatement *pstem)
        {
        }
        template <class T, class... Args>
        void bindParams(sql::PreparedStatement *pstem, int index, T &&value, Args &&...args)
        {
            pstem->setString(index, std::to_string(std::forward<T>(value)));
            bindParams(pstem, std::forward<Args>(args)...);
        }
        std::shared_ptr<sql::Connection> conn_;
        std::string host_;
        std::string user_;
        std::string password_;
        std::string database_;
    };

};