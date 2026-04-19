#pragma once
#include <string>
#include <unordered_map>

namespace http
{
    class HttpResponse
    {
    public:
        operator std::string() const
        {
        }
        std::string version_;
        std::string status_;
        std::string message_;
        std::unordered_map<std::string, std::string> headers_;
        std::string body_;
    };

} // namespace http
