#include "router.h"

namespace http
{
    namespace router
    {
        void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerCallback handlerCallback)
        {
            RouteKey key{method, path};
            handlers_[key] = handlerCallback;
        }
        bool Router::route(const HttpRequest &httprequest, HttpResponse *httpresponse)
        {
            RouteKey key{httprequest.method_, httprequest.path_};
            auto it = handlers_.find(key);
            if (it != handlers_.end())
            {
                it->second(httprequest, httpresponse);
                return true;
            }
            std::smatch match;
            for (auto &[method, regex, handlerCallback] : regexHandlers_)
            {
                if (httprequest.method_ == method && std::regex_match(httprequest.path_, match, regex))
                {
                    handlerCallback(httprequest, httpresponse);
                    return true;
                }
            }
            return false;
        }
        void Router::registerHandler(HttpRequest::Method method, const std::regex &reg, HandlerCallback handlerCallback)
        {
            regexHandlers_.emplace_back(method, reg, handlerCallback);
        }

    }
}