#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/RateLimiter.h>
#include <filters/RateLimiter.h>
#include <mutex>

using namespace drogon;

void SlimyTube::RateLimiter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb) {
    std::string client_id = req->getPeerAddr().toIp();
    std::lock_guard lk(rateLimitsMutex);

    if(rateLimits.count(client_id)) {
        if(!rateLimits.at(client_id)->isAllowed()) {
            fcb(HttpResponse::newHttpResponse(k429TooManyRequests, CT_NONE));
            return;
        }
    } else {
        rateLimits.emplace(client_id, drogon::RateLimiter::newRateLimiter(limiterType, 3, std::chrono::seconds(10)));
    }

    fccb();
}
