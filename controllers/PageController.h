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
        // errors:
        // success: 200
        ADD_METHOD_TO(PageController::main, "/", Get);

        // errors:
        // success: 200
        ADD_METHOD_TO(PageController::watch, "/watch/{1}", Get);

        // errors:
        // success: 200
        ADD_METHOD_TO(PageController::search, "/search", Get);

        // errors:
        // success: 200(login.csp - logged out) 200(account.csp - logged in)
        ADD_METHOD_TO(PageController::account, "/account", Get, "SlimyTube::LaxAuthenticator");
    METHOD_LIST_END
        void main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& video_id) const;
        void search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
