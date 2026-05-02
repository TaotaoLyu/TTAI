#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace chat
{
    class AIStrategy
    {
        using json = nlohmann::json;

    public:
        AIStrategy(const std::string &model, const std::string &api, const std::string &url)
            : url_(url), model_(model), api_(api)
        {
        }

        virtual std::string getMessage(const json &data) const = 0;
        virtual std::string getMessage(const json &data, size_t index, std::string *role) const=0;
        size_t getMessageSize(const json &data) const 
        {
            return data["input"]["messages"].size();
        }
        virtual void addMessage(json &body, const std::string &role, const std::string &message) const = 0;
        virtual json initBody() const = 0;
        virtual void setOnlineSearch(json &body, bool flag) const = 0;
        virtual void setThinking(json &body, bool flag) const = 0;
        std::string getAPI() { return api_; }
        std::string getURL() { return url_; }

    protected:
        std::string url_;
        std::string model_;
        std::string api_;
    };



    class AliMultimodal : public AIStrategy
    {
        using json = nlohmann::json;

    public:
        AliMultimodal(const std::string &model, const std::string &api, const std::string &url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation")
            : AIStrategy(model, api, url)
        {
        }

        virtual std::string getMessage(const json &data) const override;
        virtual std::string getMessage(const json &data, size_t index, std::string *role) const override;

        virtual void addMessage(json &body, const std::string &role, const std::string &message) const override;
        virtual json initBody() const override;
        virtual void setOnlineSearch(json &bod, bool flag) const override;
        virtual void setThinking(json &body, bool flag) const override;
    };

    class AliTextGen : public AIStrategy
    {
        using json = nlohmann::json;

    public:
        AliTextGen(const std::string &model, const std::string &api, const std::string &url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation")
            : AIStrategy(model, api, url)
        {
        }

        virtual std::string getMessage(const json &data) const override;
        virtual std::string getMessage(const json &data, size_t index, std::string *role) const override;

        virtual void addMessage(json &body, const std::string &role, const std::string &message) const override;
        virtual json initBody() const override;
        virtual void setOnlineSearch(json &bod, bool flag) const override;
        virtual void setThinking(json &body, bool flag) const override;
    };

}