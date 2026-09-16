#pragma once
#include <drogon/HttpFilter.h>
#include <drogon/HttpRequest.h>
#include <drogon/RateLimiter.h>
#include <drogon/drogon_callbacks.h>

#include <mutex>
#include <trantor/net/InetAddress.h>
#include <unordered_map>

using
    drogon::HttpFilter,
    drogon::HttpRequestPtr,
    drogon::FilterCallback,
    drogon::FilterChainCallback;

namespace SlimyTube {
    class RateLimiter : public drogon::HttpFilter<RateLimiter> {
        static constexpr drogon::RateLimiterType limiterType = drogon::RateLimiterType::kSlidingWindow;
        std::mutex rateLimitsMutex;
        std::unordered_map<std::string, drogon::RateLimiterPtr> rateLimits;
        public:
        RateLimiter() {}
        void doFilter(const HttpRequestPtr &req,
            FilterCallback &&fcb,
            FilterChainCallback &&fccb) override;
    };
}
