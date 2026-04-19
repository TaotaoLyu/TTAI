#include "ssl_context.h"
#include <muduo/base/Logging.h>

namespace ssl
{
    SslContext::SslContext(const SslConfig &sslConfig)
        : sslContext_(nullptr)
    {
        // create new method and use it to create sslContext
        const SSL_METHOD *method = TLS_server_method();
        sslContext_ = SSL_CTX_new(method);
        if (!sslContext_)
        {
            LOG_FATAL << "SSL_CTX_new failed!!!";
            exit(1);
        }

        //  证书
        //  私钥
        //  校验策略
        //  协议选项
        // base64 
        if(SSL_CTX_use_certificate_file(sslContext_,sslConfig.certFile_.c_str(),SSL_FILETYPE_PEM)<=0)
        {
            LOG_FATAL << "SSL_CTX_use_certificate_file failed!!!";
            exit(1);
        }
        if(SSL_CTX_use_PrivateKey_file(sslContext_,sslConfig.keyFile_.c_str(),SSL_FILETYPE_PEM)<=0)
        {
            LOG_FATAL << "SSL_CTX_use_certificate_file failed!!!";
            exit(1);
        }
        if(SSL_CTX_check_private_key(sslContext_)!=1)
        {
            LOG_FATAL << "SSL_CTX_check_private_key failed!!!";
            exit(1);
        }
        else
        {
            LOG_INFO << "SSL_CTX_check_private_key success!!!";
        }

    }
    SslContext::~SslContext()
    {
        if (sslContext_)
            SSL_CTX_free(sslContext_);
    }

}