#include"http_server.h"
#include"db_connection_pool.h"
#include<iostream>
#include<fstream>
#include<nlohmann/json.hpp>

#include "ssl_config.h"
#include"chat_server.h"
void Usage()
{
    printf("\nUsage:\n\n\t./TTAI config.json\n\n");
}

int main(int argc,char* argv[])
{
    // if(argc!=2)
    // {
    //     Usage();
    //     return 0;
    // }
    try
    {
        std::unique_ptr<chat::ChatServer> sevr=std::make_unique<chat::ChatServer>("/home/ltt/TTAI/build/config.json");


        // auto ret=sevr->userService_->insertUser("taota","123","sssss");
        // std::cout<<ret<<std::endl;
        // auto ret=sevr->userService_->getUserId("sssss");
        // std::cout<<"id: "<<ret<<std::endl;

        printf("hello");
        sevr->start();
        printf("hello");



  
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    



    return 0;
}











