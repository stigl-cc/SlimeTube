#include "controllers/RecordController.h"
#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>

#include <models/RecordManager.hh>
#include <models/Attributes.hh>
#include <models/Settings.hh>
#include <models/Session.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <stdexcept>
#include <unordered_map>
#include <charconv>
#include <string>
#include <sstream>
#include <chrono>

using SlimyTube::Attributes::LoginMethod;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;
using namespace std::chrono;

static const std::vector<std::pair<int64_t, char>> timeUnits = {
        { 31'556'952, 'y' },
        {  2'628'000, 'M' },
        {    604'800, 'w' },
        {     86'400, 'd' },
        {      3'600, 'h' },
        {         60, 'm' },
    };

std::string formatVideoAge(int64_t uploadTime) {
    const int64_t
        current_time = time_point_cast<std::chrono::seconds>(system_clock::now()).time_since_epoch().count(),
        video_age = current_time - uploadTime;

    constexpr size_t age_str_size = 5; // maximum of 9999years... propably enough
    char age_str[age_str_size] = {};

    std::pair<int64_t, char> used_unit = { 1, 's' };

    // find appropriate unit
    for(const std::pair<int64_t, char>& time_unit : timeUnits) {
        if(video_age >= time_unit.first) {
            used_unit = time_unit;
            break;
        }
    }

    // convert to string
    std::to_chars_result result = std::to_chars(age_str, age_str + age_str_size - 1, static_cast<int64_t>(video_age / used_unit.first));
    if(result.ec != std::errc())
        throw std::runtime_error("Failed to convert video age: " +  std::make_error_code(result.ec).message());

    // add unit mark
    *result.ptr++ = used_unit.second;
    return std::string(age_str, result.ptr);
}

static const std::vector<std::pair<int64_t, char>> viewCountUnits = {
    { 1'000'000'000, 'B' },
    {     1'000'000, 'M' },
    {         1'000, 'k' },
};

std::string formatViewCount(int64_t viewCount) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    std::pair<int64_t, char> used_unit = { 1, 0 };

    // find appropriate unit
    for(const std::pair<int64_t, char>& view_count_unit : viewCountUnits) {
        if(viewCount >= view_count_unit.first) {
            used_unit = view_count_unit;
            break;
        }
    }

    ss << viewCount / static_cast<float>(used_unit.first);
    if(used_unit.second) ss << used_unit.second;

    return ss.str();
}

std::vector<std::unordered_map<std::string, std::string>> CreatePreviews(size_t amount) {
    Mapper<Users> user_mapper = SlimyTube::RecordManager::getUserMapper();
    Mapper<Videos> video_mapper = SlimyTube::RecordManager::getVideosMapper();
    std::vector<Videos> videos = video_mapper.limit(amount).findAll();

    std::vector<std::unordered_map<std::string, std::string>> previews;
    for(const Videos& video : videos) {
        std::string viewCountStr = formatViewCount(video.getValueOfViews());
        std::string ageStr = formatVideoAge(video.getValueOfCreationTime());

        previews.emplace_back(std::unordered_map<std::string, std::string> {
                {"id", video.getValueOfId()},
                {"length", "20:23"},
                {"title", HttpViewData::htmlTranslate(video.getValueOfTitle())},
                {"channel",  user_mapper.findByPrimaryKey(video.getValueOfChannel()).getValueOfName()},
                {"views", viewCountStr },
                {"age", ageStr },
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
    HttpViewData data = Settings::getInstance().getData();
    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();

    Criteria crit = Criteria(Videos::Cols::_id, CompareOperator::EQ, video_id);
    std::vector<Videos> videos = video_mapper.findBy(crit);
    if(videos.empty()) {
        respond(HttpResponse::newHttpViewResponse("error404.csp", data));
        return;
    }

    const Videos& video = videos[0];
    const Users& channel = video.getUser(RecordManager::getDbClient());
    const std::string
        viewCountStr = formatViewCount(video.getValueOfViews()),
        ageStr = formatVideoAge(video.getValueOfCreationTime()),
        title = HttpViewData::htmlTranslate(video.getValueOfTitle()),
        desc = HttpViewData::htmlTranslate(video.getValueOfDescription()),
        user_name = HttpViewData::htmlTranslate(channel.getValueOfName()),
        user_id = std::to_string(channel.getValueOfId());

    std::vector<std::unordered_map<std::string, std::string>> previews = CreatePreviews(25);;

    data.insert("video_id", video_id);
    data.insert("previews", previews);

    data.insert("video_views", viewCountStr);
    data.insert("video_age", ageStr);
    data.insert("video_title", title);
    data.insert("video_description", desc);
    data.insert("video_channel_name", user_name);
    data.insert("video_channel_id", user_id);

    respond(HttpResponse::newHttpViewResponse("watch.csp", data));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();

    LoginMethod loginMethod = req->attributes()->get<LoginMethod>(Attributes::keyLoginMethod.data());
    std::vector<std::unordered_map<std::string, std::string>> video_entries;

    if(loginMethod == LoginMethod::LaxNone) {
        data.insert("message", HttpViewData::htmlTranslate(req->getParameter("m")));
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
                { "title", HttpViewData::htmlTranslate(video.getValueOfTitle()) },
                { "description", HttpViewData::htmlTranslate(video.getValueOfDescription()) },
                { "size", "500.0mb" },
            });
    }

    data.insert("user_id", std::to_string(userId));
    data.insert("user_name", user.getValueOfName());
    data.insert("videos", video_entries);

    respond(HttpResponse::newHttpViewResponse("account.csp", data));
}
