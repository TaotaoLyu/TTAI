#pragma once
#include <functional>
#include<vector>
#include<regex>
#include <unordered_map>
#include "http_request.h"
#include "http_response.h"

namespace http
{
    namespace router
    {

        class Router
        {
public:
            using HandlerCallback = std::function<void(const HttpRequest &httprequest, HttpResponse *httpresponse)>;
private:
            struct RouteKey
            {
                HttpRequest::Method method_;
                std::string path_;
                bool operator==(const RouteKey &routekey) const
                {
                    return method_ == routekey.method_ && path_ == routekey.path_;
                }
                size_t operator()(const RouteKey &routekey) const
                {
                    size_t methodHash = std::hash<int>{}(static_cast<int>(method_));
                    size_t pathHash = std::hash<std::string>{}(path_);
                    return methodHash * 31 + pathHash;
                }
            };
            struct RegexRouteKey
            {
                HttpRequest::Method method_;
                std::regex regex_;
                HandlerCallback handlerCallback_;
                RegexRouteKey(HttpRequest::Method method, const std::regex &reg, HandlerCallback handlerCallback)
                    :method_(method),regex_(reg),handlerCallback_(handlerCallback)
                {

                }
            };


        public:
            void registerHandler(HttpRequest::Method method, const std::string& path,HandlerCallback handlerCallback);
            void registerHandler(HttpRequest::Method method, const std::regex& path,HandlerCallback handlerCallback);
            // void registerRegexHandler(HttpRequest::Method method, const std::regex& reg,HandlerCallback handlerCallback);

            bool route(const HttpRequest &httprequest, HttpResponse *httpresponse);
        public:
            Router() = default;
            ~Router() = default;

        private:
            std::unordered_map<RouteKey, HandlerCallback, RouteKey> handlers_;
            std::vector<RegexRouteKey> regexHandlers_;
        };
    }
}