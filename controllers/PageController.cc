#include "PageController.h"

#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <string>
#include <unordered_map>

using drogon::HttpViewData, drogon::HttpResponse;

void SlimyTube::PageController::main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {
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

    HttpResponsePtr res = HttpResponse::newHttpViewResponse("main.csp", data);
    callback(res);
}

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& video_id) const {
    HttpViewData data;
    data.insert("title", "SlimyTube");
    data.insert("video_id", video_id);

    HttpResponsePtr res = HttpResponse::newHttpViewResponse("watch.csp", data);
    callback(res);
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {

}
