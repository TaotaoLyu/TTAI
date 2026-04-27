#pragma once
#include <string>
#include <fstream>

class FileUtil
{
public:
    static void ReadFile(const std::string &filename, std::string &str)
    {

        std::ifstream fin(filename, std::ios::binary);
        fin.seekg(0, std::ios::end);
        size_t fileSize = fin.tellg();
        fin.seekg(0, std::ios::beg);
        str.resize(fileSize);
        if (fileSize > 0)
        {
            fin.read(&str[0], fileSize);
        }
    }

    static std::string FileType(const std::string &filename)
    {
        if (filename.find(".html") != std::string::npos)
        {
            return "text/html";
        }
        else if (filename.find(".css") != std::string::npos)
        {
            return "text/css";
        }
        else if (filename.find(".js") != std::string::npos)
        {
            return "application/javascript";
        }
        else if (filename.find(".png") != std::string::npos)
        {
            return "image/png";
        }
        else if (filename.find(".jpg") != std::string::npos)
        {
            return "image/jpeg";
        }
        else if (filename.find(".jpeg") != std::string::npos)
        {
            return "image/jpeg";
        }
        else if (filename.find(".json") != std::string::npos)
        {
            return "application/json";
        }
        else
        {
            return "text/plain";
        }
    }
};