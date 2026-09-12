#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>

#include <models/Attributes.hh>
#include <models/Session.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <unordered_map>
#include <string>

using namespace drogon;
using namespace drogon_model::SlimyTube;

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
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    uint32_t userId = req->attributes()->get<uint32_t>(Attributes::keyUserId.data());
    std::vector<std::unordered_map<std::string, std::string>> video_entries;

    const Users user = user_mapper.findByPrimaryKey(userId);
    const std::vector<Videos> videos = user.getVideos(db);

    for(const Videos& video : videos) {
        video_entries.emplace_back(std::unordered_map<std::string, std::string>{
                { "id", video.getValueOfId() },
                { "title", video.getValueOfTitle() },
                { "description", video.getValueOfDescription() },
                { "size", video.getValueOfDescription() },
            });
    }

    video_entries.emplace_back(std::unordered_map<std::string, std::string>{
            { "id", "asdj" },
            { "title", "Very interesting video title" },
            { "description", "Wat is this description thing anyway" },
            { "size", "571.3MB" },
        });

    HttpViewData data;
    data.insert("title", "SlimyTube");
    data.insert("user_id", userId);
    data.insert("user_name", user.getValueOfName());
    data.insert("videos", videos);

    respond(HttpResponse::newHttpViewResponse("account.csp", data));
}
