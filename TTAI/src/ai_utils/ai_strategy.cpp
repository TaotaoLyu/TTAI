#include "ai_strategy.h"
#include <muduo/base/Logging.h>

namespace chat
{
    std::string AliMultimodal::getMessage(const json &data) const
    {
        try
        {
            if (data["output"]["choices"].size() >= 1 && data["output"]["choices"][0]["message"]["content"].size() >= 1)
            {
                return data["output"]["choices"][0]["message"]["content"][0]["text"].get<std::string>();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "getMessage error: " << e.what();
            throw;
        }
        return "";
    }
    std::string AliMultimodal::getMessage(const json &data, size_t index, std::string *role) const
    {
        try
        {
            *role = data["input"]["messages"][index]["role"];
            return data["input"]["messages"][index]["content"][0]["text"];
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "getMessage error: " << e.what();
            throw;
        }
        return "";
    }

    void AliMultimodal::addMessage(json &body, const std::string &role, const std::string &message) const
    {
        auto cur = body["input"]["messages"].size();
        body["input"]["messages"][cur]["role"] = role;
        body["input"]["messages"][cur]["content"][0]["text"] = message;
    }
    AliMultimodal::json AliMultimodal::initBody() const
    {
        json body;
        body["model"] = model_;
        body["parameters"]["enable_thinking"] = false;
        body["parameters"]["enable_search"] = false;
        body["input"]["messages"][0]["role"] = "system";
        body["input"]["messages"][0]["content"][0]["text"] = "You are a helpful assistant.";
        body["stream"] = true;
        body["stream_options"]["include_usage"] = true;
        return body;
    }

    void AliMultimodal::setOnlineSearch(json &body, bool flag) const
    {
        body["parameters"]["enable_search"] = flag;
    }
    void AliMultimodal::setThinking(json &body, bool flag) const
    {
        body["parameters"]["enable_thinking"] = flag;
    }

    // ======================================================
    std::string AliTextGen::getMessage(const json &data) const
    {
        try
        {
            if (data["output"]["choices"].size() >= 1)
            {
                return data["output"]["choices"][0]["message"]["content"].get<std::string>();
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "getMessage error: " << e.what();
            throw;
        }
        return "";
    }

    std::string AliTextGen::getMessage(const json &data, size_t index, std::string *role) const
    {
        try
        {
            *role = data["input"]["messages"][index]["role"];
            return data["input"]["messages"][index]["content"];
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "getMessage error: " << e.what();
            throw;
        }
        return "";
    }

    void AliTextGen::addMessage(json &body, const std::string &role, const std::string &message) const
    {
        auto cur = body["input"]["messages"].size();
        body["input"]["messages"][cur]["role"] = role;
        body["input"]["messages"][cur]["content"] = message;
    }
    AliTextGen::json AliTextGen::initBody() const
    {
        json body;
        body["model"] = model_;
        body["parameters"]["enable_thinking"] = false;
        body["parameters"]["enable_search"] = false;
        body["input"]["messages"][0]["role"] = "system";
        body["input"]["messages"][0]["content"] = "You are a helpful assistant.";
        body["stream"] = true;
        body["stream_options"]["include_usage"] = true;
        return body;
    }

    void AliTextGen::setOnlineSearch(json &body, bool flag) const
    {
        body["parameters"]["enable_search"] = flag;
    }
    void AliTextGen::setThinking(json &body, bool flag) const
    {
        body["parameters"]["enable_thinking"] = flag;
    }

}