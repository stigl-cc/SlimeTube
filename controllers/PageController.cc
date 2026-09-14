#include <Settings.hh>
#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>

#include <models/RecordManager.hh>
#include <models/Attributes.hh>
#include <models/Session.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <unordered_map>
#include <string>
#include <sstream>

using SlimyTube::Attributes::LoginMethod;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;

std::vector<std::unordered_map<std::string, std::string>> CreatePreviews(size_t amount) {
    Mapper<Users> user_mapper = SlimyTube::RecordManager::getUserMapper();
    Mapper<Videos> video_mapper = SlimyTube::RecordManager::getVideosMapper();
    std::vector<Videos> videos = video_mapper.limit(amount).findAll();

    std::vector<std::unordered_map<std::string, std::string>> previews;
    for(const Videos& video : videos) {
        int64_t views = video.getValueOfViews();

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        if(views >= 1'000'000) {
            ss << views / 1'000'000.0f;
        }
        else if(views >= 1'000){
            ss << views / 1'000.0f;
        }
        else {
            ss << views;
        }

        previews.emplace_back(std::unordered_map<std::string, std::string> {
                {"id", video.getValueOfId()},
                {"length", "20:23"},
                {"title", HttpViewData::htmlTranslate(video.getValueOfTitle())},
                {"channel",  user_mapper.findByPrimaryKey(video.getValueOfChannel()).getValueOfName()},
                {"views", ss.str()},
                {"age", "2y"},
            });
    }
    return previews;
}

void SlimyTube::PageController::main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    std::vector<std::unordered_map<std::string, std::string>> previews = CreatePreviews(25);

    HttpViewData data = Settings::getInstance().getData();
    data.insert("previews", previews);

    respond(HttpResponse::newHttpViewResponse("main.csp", data));
}

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& video_id) const {
    std::vector<std::unordered_map<std::string, std::string>> previews = CreatePreviews(25);;
    //TODO: Add HttpViewData::htmlTranslate(""); to everything

    HttpViewData data = Settings::getInstance().getData();
    data.insert("video_id", video_id);
    data.insert("previews", previews);

    data.insert("video_title", HttpViewData::htmlTranslate("A cool video!"));
    data.insert("video_views", "200k");
    data.insert("video_age", "2y");
    data.insert("video_description", HttpViewData::htmlTranslate("A cool video!"));

    respond(HttpResponse::newHttpViewResponse("watch.csp", data));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();

    LoginMethod loginMethod = req->attributes()->get<LoginMethod>(Attributes::keyLoginMethod.data());
    std::vector<std::unordered_map<std::string, std::string>> video_entries;

    if(loginMethod == LoginMethod::LaxNone) {
        data.insert("message", req->getParameter("m"));
        respond(HttpResponse::newHttpViewResponse("login.csp", data));
        return;
    }

    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    uint32_t userId = req->attributes()->get<uint32_t>(Attributes::keyUserId.data());
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

    data.insert("user_id", std::to_string(userId));
    data.insert("user_name", user.getValueOfName());
    data.insert("videos", video_entries);

    respond(HttpResponse::newHttpViewResponse("account.csp", data));
}
