#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpSimpleController.h>

using drogon::HttpRequestPtr,
    drogon::HttpResponsePtr,
    drogon::HttpMethod::Get;

namespace SlimyTube {
    class PageController : public drogon::HttpController<PageController> {
        public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PageController::main, "/", Get);
        ADD_METHOD_TO(PageController::watch, "/watch/{1}", Get);
        ADD_METHOD_TO(PageController::account, "/account", Get, "SlimyTube::Authenticator");
    METHOD_LIST_END
        void main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& video_id) const;
        void account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
