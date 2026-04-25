#include "db_connection_pool.h"

namespace db
{
    void DbConnectionPool::init(const std::string &host,
                                const std::string &user,
                                const std::string &password,
                                const std::string &database,
                                uint32_t connNum)
    {

        try
        {
            std::lock_guard<std::mutex> lg(mutex_);
            if (initialized_)
                return;
            host_ = host;
            user_ = user;
            password_ = password;
            database_ = database;
            connNum_ = std::max(static_cast<uint32_t>(1),connNum);
            for (int i = 0; i < connNum_; ++i)
            {
                connections_.push(createConnection());
            }
            initialized_ = true;

            std::thread thread_(&DbConnectionPool::check,this);
            thread_.detach();
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "init error..." << e.what();
            throw;
        }
    }

    std::shared_ptr<DbConnection> DbConnectionPool::createConnection()
    {
        try
        {
            return std::make_shared<DbConnection>(host_, user_, password_, database_);
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "createConnection error..." << e.what();
            throw;
        }
    }
    std::shared_ptr<DbConnection> DbConnectionPool::getConnection()
    {
        std::shared_ptr<DbConnection> conn_;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&]()
                     { return !connections_.empty(); });
            conn_ = connections_.front();
            connections_.pop();
        }

        try
        {
            if (!conn_->ping())
            {
                LOG_WARN << "Connection lost, attempting to reconnect...";
                conn_->reconnect();
            }

            return std::shared_ptr<DbConnection>(conn_.get(), [this, conn_](DbConnection *connptr)
                                                 {
            std::lock_guard<std::mutex> lg(mutex_);
            connections_.push(conn_);
            cv_.notify_one(); });
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << " failed to reconnect: " << e.what();
            {
                std::lock_guard<std::mutex> lg(mutex_);
                connections_.push(conn_);
                cv_.notify_one();
            }
            throw;
        }
    }
    void DbConnectionPool::check()
    {

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::shared_ptr<DbConnection> conn;
            {
                std::lock_guard<std::mutex> lg(mutex_);
                if (connections_.empty())
                {
                    continue;
                }
                conn = connections_.front();
                connections_.pop();
            }
            if (!conn->ping())
            {
                try
                {
                    conn->reconnect();
                }
                catch (const sql::SQLException &e)
                {
                    LOG_ERROR << "Connection Pool has one connection which failed to reconnect: " << e.what();
                }
            }
            {
                std::lock_guard<std::mutex> lg(mutex_);
                connections_.push(conn);
            }
            cv_.notify_one();
        }
    }
}
