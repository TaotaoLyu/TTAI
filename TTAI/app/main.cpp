#include "http_server.h"
#include "db_connection_pool.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <nlohmann/json.hpp>

#include "ssl_config.h"
#include "chat_server.h"

void daemonize(const std::string &logPath = "/dev/null")
{
    pid_t pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);
    std::cout << "daemon started, pid = " << getpid() << std::endl;
    umask(0);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    // open(logpath.c_str(), O_WRONLY | O_APPEND);
    if (open(logPath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644) < 0)
        std::cout << "log file error, exit...";
    open("/dev/null", O_RDWR);
    LOG_INFO << "server started, pid[" << getpid() << "]";
}

std::unique_ptr<chat::ChatServer> sevr = nullptr;

void signalHandler(int signo)
{
    try
    {
        sevr->flushTodisk();
        exit(EXIT_SUCCESS);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "exit error" << e.what();
        exit(EXIT_FAILURE);
    }
}
void Usage()
{
    printf("\nUsage:\n\n\t./TTAI config.json\n\n");
}

int main(int argc, char *argv[])
{

    // if(argc!=2)
    // {
    //     Usage();
    //     return 0;
    // }
    try
    {
        std::ifstream fin("/home/ltt/TTAI/build/config.json");
        nlohmann::json config;
        fin >> config;
        fin.close();
        std::string logPath = config["log"].value("file", "/dev/null");
        daemonize(logPath);
        // std::unique_ptr<chat::ChatServer> sevr = std::make_unique<chat::ChatServer>("/home/ltt/TTAI/build/config.json");
        // sevr = std::make_unique<chat::ChatServer>("/home/ltt/TTAI/build/config.json");
        sevr = std::make_unique<chat::ChatServer>(config);
        signal(SIGINT, signalHandler);

        sevr->start();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
