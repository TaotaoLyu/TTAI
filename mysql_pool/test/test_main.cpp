#include<iostream>
#include"db_connection.h"
#include"db_connection_pool.h"

int main()
{
    // todo try
    // db::DbConnection dbc("127.0.0.1:3306","ltt","ltt","pool_test");
    // db::DbConnectionPool dbp("127.0.0.1:3306","ltt","ltt","pool_test");
    db::DbConnectionPool::getInstance().init("127.0.0.1:3306","ltt","ltt","pool_test",10);

    auto ret=db::DbConnectionPool::getInstance().executeQuery("select * from conn_pool_demo where id=?",1,2);
    // auto ret=dbc.executeQuery("select * from conn_pool_demo where id=?",1,2);
    std::cout<<"\nresult:\n";
    while (ret->next())
    {
        int id=ret->getInt("id");
        std::string name=ret->getString("name");
        int score=ret->getInt("score");
        std::cout<<id<<" "<<name<<" "<<score<<std::endl;
    }
    
    
    std::cout<<"hello";
    return 0;
}