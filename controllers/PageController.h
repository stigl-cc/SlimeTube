#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/HttpTypes.h>

#include <unordered_map>

namespace SlimyTube {
    using drogon::HttpRequestPtr,
        drogon::HttpResponsePtr,
        drogon::HttpMethod::Get,
        drogon::HttpMethod::Post;

    class PageController : public drogon::HttpController<PageController> {
        private:
        std::vector<std::unordered_map<std::string, std::string>> createPreviews(const std::string& ignore, size_t count) const;
        public:
    METHOD_LIST_BEGIN
        // errors:
        // success: 200
        ADD_METHOD_TO(PageController::main, "/", Get);

        // errors: 404
        // success: 200
        ADD_METHOD_TO(PageController::watch, "/watch/{1}", Get);
        ADD_METHOD_TO(PageController::watch, "/watch/?v={1}", Get);

        // errors:
        // success: 200
        ADD_METHOD_TO(PageController::search, "/search", Get);

        // errors:
        // success: 200(login.csp - logged out) 200(account.csp - logged in)
        ADD_METHOD_TO(PageController::account, "/account", Get, "SlimyTube::LaxAuthenticator");

        ADD_METHOD_TO(PageController::reg, "/register", Get);

        ADD_METHOD_TO(PageController::create, "/create", Get, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(PageController::create, "/create", Post, "SlimyTube::StrictAuthenticator");

        ADD_METHOD_TO(PageController::upload, "/video/upload/{1}", Get, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(PageController::edit, "/video/edit/{1}", Get, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END
        void main(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;
        void watch(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&, const std::string& video_id) const;
        void search(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;
        void account(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;

        void reg(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;
        void create(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;

        void upload(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&, const std::string& video_id) const;
        void edit(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&, const std::string& video_id) const;
        static void error(std::function<void (const HttpResponsePtr &)>&&, drogon::HttpStatusCode err, const std::string& back);
    };
}
