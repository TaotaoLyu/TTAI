#pragma once
#include "ai_strategy.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include<mutex>

namespace chat
{
    class StrategyFactory
    {
        using Creator = std::function<std::shared_ptr<AIStrategy>()>;

    public:
        static StrategyFactory &getInstance()
        {
            static StrategyFactory ret;
            return ret;
        }
        void registerStrategy(const std::string &name, const Creator &creator)
        {
            creators_[name] = creator;
        }
        std::shared_ptr<AIStrategy> getStrategy(const std::string &name)
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            auto it = cache_.find(name);

            if (it != cache_.end())
                return it->second;
       
            auto p = creators_[name]();

            cache_[name] = p;

            return p;
        }
        ~StrategyFactory() = default;

    private:
        /* data */
        StrategyFactory() = default;
        std::unordered_map<std::string, Creator> creators_;
        std::unordered_map<std::string, std::shared_ptr<AIStrategy>> cache_;
        std::mutex cacheMutex_;
    };

    template <class T>
    struct StrategyRegister
    {
        StrategyRegister(const std::string &name, const std::string &model, const std::string &api, const std::string &url)
        {
            StrategyFactory::getInstance().registerStrategy(name, [model,api,url]() -> std::shared_ptr<T>
                                                            { 
                                                                auto ret=std::make_shared<T>(model, api, url);
                                                                
                                                                
                                                                return  ret;});
        }
    };

} // namespace chat
