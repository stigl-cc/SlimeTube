#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>

using namespace drogon;

namespace SlimyTube {
    class PageController : public drogon::HttpController<PageController> {
        public:
    METHOD_LIST_BEGIN
        METHOD_ADD(PageController::main, "/", Get);
        METHOD_ADD(PageController::watch, "/watch/{1}", Get);
        METHOD_ADD(PageController::account, "/account", Get);
    METHOD_LIST_END
        void main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string arg) const;
    };
}
