#include "PageController.h"

#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <string>
#include <unordered_map>

using namespace drogon;

void SlimyTube::PageController::main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    std::vector<std::unordered_map<std::string, std::string>> previews;
    for(int i =0; i < 23; i++) {
        previews.emplace_back(std::unordered_map<std::string, std::string>{
                {"id", "12345"},
                {"length", "20:23"},
                {"title", "Some title lool"},
                {"channel", "channel123"},
                {"views", "200k"},
                {"age", "2y"},
            });
    }

    HttpViewData data;
    data.insert("title", "SlimyTube");
    data.insert("previews", previews);

    respond(HttpResponse::newHttpViewResponse("main.csp", data));
}

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& video_id) const {
    std::vector<std::unordered_map<std::string, std::string>> previews;
    for(int i =0; i < 23; i++) {
        previews.emplace_back(std::unordered_map<std::string, std::string>{
                {"id", "12345"},
                {"length", "20:23"},
                {"title", "Some title lool"},
                {"channel", "channel123"},
                {"views", "200k"},
                {"age", "2y"},
            });
    }

    HttpViewData data;
    data.insert("title", "SlimyTube");
    data.insert("video_id", video_id);
    data.insert("previews", previews);

    data.insert("video_title", "A cool video!");
    data.insert("video_views", "200k");
    data.insert("video_age", "2y");
    data.insert("video_description", "A cool video!");

    respond(HttpResponse::newHttpViewResponse("watch.csp", data));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {

}
