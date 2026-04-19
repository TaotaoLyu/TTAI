#pragma once
#include <openssl/ssl.h>
#include "ssl_config.h"

namespace ssl
{
    class SslContext
    {
    public:
        SslContext(const SslConfig &sslConfig);
        SslContext()=default;
        SSL_CTX *getSslContext() { return sslContext_; }
        ~SslContext();

    private:
        SSL_CTX *sslContext_;
    };
}