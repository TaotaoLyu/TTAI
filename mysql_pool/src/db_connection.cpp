#include "db_connection.h"

namespace db
{
    DbConnection::DbConnection(const std::string &host,
                               const std::string &user,
                               const std::string &password,
                               const std::string &database)
        : host_(host),
          user_(user),
          password_(password),
          database_(database)
    {
        try
        {
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
            conn_.reset(driver->connect("tcp://" + host_, user_, password_));
            if (conn_)
            {
                conn_->setSchema(database_);
                LOG_INFO << "Database connection established";
            }
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "Failed to create database connection: " << e.what();
            throw;
        }
    }
    void DbConnection::reconnect()
    {
        try
        {
            if (conn_)
            {
                conn_->reconnect();
            }
            else
            {
                sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
                conn_.reset(driver->connect("tcp://" + host_, user_, password_));
                if (conn_)
                {
                    conn_->setSchema(database_);
                    LOG_INFO << "Database reconnection success";
                }
            }
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "reconnection error: " << e.what();
            throw;
        }
    }

    bool DbConnection::ping()
    {
        try
        {
            std::unique_ptr<sql::Statement> stem(conn_->createStatement());
            std::unique_ptr<sql::ResultSet> res(stem->executeQuery("select 1"));
            return true;
        }
        catch (const sql::SQLException &e)
        {
            // LOG_ERROR << "ping error: " << e.what();
            // throw e;
            return false;
        }
    }

    DbConnection::~DbConnection()
    {
    }
}