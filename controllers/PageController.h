#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/HttpTypes.h>

#include <unordered_map>
#include <utility>
#include <cstdint>
#include <array>

namespace SlimyTube {
    using drogon::HttpRequestPtr,
        drogon::HttpResponsePtr,
        drogon::HttpMethod::Get,
        drogon::HttpMethod::Post;

    class PageController : public drogon::HttpController<PageController> {
        private:
        static constexpr std::array<std::pair<int64_t, char>, 6> timeUnits {{
            { 31'556'952, 'y' },
            {  2'628'000, 'M' },
            {    604'800, 'w' },
            {     86'400, 'd' },
            {      3'600, 'h' },
            {         60, 'm' },
        }};

        static constexpr std::array<std::pair<int64_t, char>, 3> viewCountUnits = {{
            { 1'000'000'000, 'B' },
            {     1'000'000, 'M' },
            {         1'000, 'k' },
        }};

        std::string formatVideoAge(int64_t uploadTime) const;
        std::string formatViewCount(int64_t viewCount) const;
        std::vector<std::unordered_map<std::string, std::string>> createPreviews(size_t count) const;
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

        ADD_METHOD_TO(PageController::create, "/create", Get, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(PageController::create, "/create", Post, "SlimyTube::StrictAuthenticator");

        ADD_METHOD_TO(PageController::upload, "/upload/{1}", Get, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END
        void main(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;
        void watch(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&, const std::string& video_id) const;
        void search(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;
        void account(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;

        void create(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&) const;

        void upload(const HttpRequestPtr&, std::function<void (const HttpResponsePtr &)>&&, const std::string& video_id) const;
        static void error(std::function<void (const HttpResponsePtr &)>&&, drogon::HttpStatusCode err, const std::string& back);
    };
}
