#pragma once
#include <string>

namespace ssl
{
    class SslConfig
    {
    public:
        SslConfig(const std::string &certFile, const std::string &keyFile)
        {
            certFile_ = certFile;
            keyFile_ = keyFile;
        }
        SslConfig() = delete;
        ~SslConfig() = default;
    public:
        // SSL configuration parameters
        std::string certFile_;
        std::string keyFile_;
    };
}