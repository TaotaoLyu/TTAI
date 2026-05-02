#include "ssl_connection.h"
#include <openssl/err.h>

namespace ssl
{
    SslConection::SslConection(SslContext &sslContext)
        : ssl_(nullptr),
          readBio_(nullptr),
          state_(kHandShake)
    {
        ssl_ = SSL_new(sslContext.getSslContext()); // todo
        if (!ssl_)
        {
            LOG_ERROR << "SSL_new failed!!!";
            state_ = kError; // need close connection
            throw std::runtime_error("ssl error");
        }
        SSL_set_accept_state(ssl_);

        readBio_ = BIO_new(BIO_s_mem());
        writeBio_ = BIO_new(BIO_s_mem());
        if (!readBio_ || !writeBio_)
        {
            LOG_ERROR << "readBio or writeBio failed!!!";
            state_ = kError; // need close connection
            throw std::runtime_error("ssl error");
        }
        SSL_set_bio(ssl_, readBio_, writeBio_);
    }
    SslConection::~SslConection()
    {
        if (ssl_)
            SSL_free(ssl_);
    }
    muduo::net::Buffer *SslConection::decrypt(muduo::net::Buffer *buffer)
    {

        if (state_ == kEstablished)
        {
            BIO_write(readBio_, buffer->peek(), buffer->readableBytes());
            buffer->retrieve(buffer->readableBytes());
            char decryptedData[4096];
            int ret = SSL_read(ssl_, decryptedData, sizeof(decryptedData));
            if (ret < 0)
            {
                int err = SSL_get_error(ssl_, ret);
                switch (err)
                {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    break; // normal, need more data to handshake
                default:
                    char errBuf[4096];
                    uint64_t errCode = ERR_get_error();
                    ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
                    LOG_ERROR << "decrypt failed: " << errBuf;
                    state_ = kError;
                    throw std::runtime_error("ssl error");
                    break;
                }
            }
            else
            {
                readBuffer_.append(decryptedData, ret);
            }
        }
        return &readBuffer_;
    }

    muduo::net::Buffer *SslConection::encrypt(const std::string& message)
    {
        int ret = SSL_write(ssl_,message.c_str(), message.size());
        if (ret <= 0)
        {
            int err = SSL_get_error(ssl_, ret);
            LOG_ERROR << "SSL_write failed, err=" << err;
            return &writeBuffer_;
        }
        flushWriteBio();
        return &writeBuffer_;
    }

    void SslConection::flushWriteBio()
    {
        char buf[4096];
        while (BIO_pending(writeBio_) > 0)
        {
            int n = BIO_read(writeBio_, buf, sizeof(buf));
            if (n > 0)
            {
                writeBuffer_.append(buf, n);
            }
            else
            {
                break;
            }
        }
    }
    void SslConection::handShake(muduo::net::Buffer *buffer)
    {
        BIO_write(readBio_, buffer->peek(), buffer->readableBytes());
        buffer->retrieve(buffer->readableBytes());
        int ret = SSL_do_handshake(ssl_);
        flushWriteBio();
        if (ret == 1)
        {
            state_ = kEstablished;
            LOG_INFO << "SSL handshake completed successfully";
        }
        else
        {
            int err = SSL_get_error(ssl_, ret);
            switch (err)
            {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // LOG_ERROR << "SSL handshake not complete";
                break; // normal, need more data to handshake
            default:
                char errBuf[4096];
                uint64_t errCode = ERR_get_error();
                ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
                LOG_ERROR << "SSL handshake failed: " << errBuf;
                state_ = kError;
                throw std::runtime_error("ssl error");

                break;
            }
        }
    }
}