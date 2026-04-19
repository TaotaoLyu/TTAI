#pragma once
#include <openssl/ssl.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>
#include "ssl_connection.h"
#include "ssl_context.h"
#include <memory>

namespace ssl
{
    enum SSLState
    {
        kHandShake,
        kEstablished,
        kShutdown,
        kError
    };

    class SslConection
    {
    public:
        SslConection(SslContext &sslContext);
        SslConection() = delete;
        ~SslConection();
        SSLState getState() { return state_; }
        bool isError() { return state_ == kError; }
        bool iskEstablished() { return state_ == kEstablished; }
        muduo::net::Buffer *decrypt(muduo::net::Buffer *buffer);
        muduo::net::Buffer *encrypt(const std::string& message);
        muduo::net::Buffer *getWriteBuffer() { return &writeBuffer_; }
        bool isDecryptionComplete() { return BIO_ctrl_pending(readBio_) == 0; }
        void handShake(muduo::net::Buffer *buffer);
        void flushWriteBio();
        int shutdown()
        {
            state_=kShutdown;
            int ret=SSL_shutdown(ssl_);
            flushWriteBio();
            return ret;
        }

    private:
        SSL *ssl_;
        BIO *readBio_;
        BIO *writeBio_;
        muduo::net::Buffer readBuffer_;
        muduo::net::Buffer writeBuffer_;
        SSLState state_;
    };
    using SslConectionPtr = std::shared_ptr<SslConection>;
}