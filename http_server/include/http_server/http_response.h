#pragma once
#include <string>
#include <sstream>
#include <unordered_map>

namespace http
{
    class HttpResponse
    {
    public:
        operator std::string &()
        {
            std::ostringstream oss;
            oss << "HTTP/" << version_ << " " << status_ << " " << describes_ << "\r\n";
            if (headers_.count("Content-Length") == 0)
                headers_["Content-Length"] = std::to_string(body_.size());
            for (auto &[key, value] : headers_)
            {
                oss << key << ": " << value << "\r\n";
            }
            oss << "\r\n";
            oss << body_;
            message_ = std::move(oss.str());
            return message_;
        }
        std::string version_ = "1.1";
        std::string status_;
        std::string describes_;
        std::unordered_map<std::string, std::string> headers_;
        std::string body_;
        std::string message_;
    };

} // namespace http
