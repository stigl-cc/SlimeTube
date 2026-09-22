#include <controllers/RecordController.h>
#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>
#include <drogon/utils/Utilities.h>
#include <lib/drogon/lib/src/HttpUtils.h>

#include <models/RecordManager.hh>
#include <models/Attributes.hh>
#include <models/Settings.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <stdexcept>
#include <unordered_map>
#include <charconv>
#include <string>
#include <sstream>
#include <chrono>
#include <utility>
#include <iomanip>

using SlimyTube::Attributes::LoginMethod;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;
using namespace std::chrono;

std::string SlimyTube::PageController::formatVideoAge(int64_t uploadTime) const {
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

std::string SlimyTube::PageController::formatViewCount(int64_t viewCount) const {
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

    if(used_unit.first == 1)
        ss << std::setprecision(0);

    ss << viewCount / static_cast<float>(used_unit.first);
    if(used_unit.second) ss << used_unit.second;

    return ss.str();
}

// TODO: Add some sort of smarter algorithm
std::vector<std::unordered_map<std::string, std::string>> SlimyTube::PageController::createPreviews(size_t amount) const {
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
    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews(25);

    HttpViewData data = Settings::getInstance().getData();
    data.insert("previews", previews);

    respond(HttpResponse::newHttpViewResponse("main.csp", std::move(data)));
}

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& video_id) const {
    HttpViewData data = Settings::getInstance().getData();

    std::optional<Videos> video = RecordManager::getVideoIfExists(video_id);
    if(!video.has_value())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/");

    const Users& channel = video.value().getUser(RecordManager::getDbClient());
    const std::string
        viewCountStr = formatViewCount(video.value().getValueOfViews()),
        ageStr = formatVideoAge(video.value().getValueOfCreationTime()),
        title = HttpViewData::htmlTranslate(video.value().getValueOfTitle()),
        desc = HttpViewData::htmlTranslate(video.value().getValueOfDescription()),
        user_name = HttpViewData::htmlTranslate(channel.getValueOfName()),
        user_id = std::to_string(channel.getValueOfId());

    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews(25);

    data.insert("video_id", video_id);
    data.insert("previews", previews);

    data.insert("video_views", viewCountStr);
    data.insert("video_age", ageStr);
    data.insert("video_title", title);
    data.insert("video_description", desc);
    data.insert("video_channel_name", user_name);
    data.insert("video_channel_id", user_id);

    respond(HttpResponse::newHttpViewResponse("watch.csp", std::move(data)));
}

void SlimyTube::PageController::search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    return(void) error(std::move(respond), HttpStatusCode::k501NotImplemented, "/");

    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews(25);

    HttpViewData data = Settings::getInstance().getData();
    data.insert("previews", previews);

    // TODO
    respond(HttpResponse::newHttpViewResponse("search.csp", std::move(data)));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();

    LoginMethod loginMethod = req->attributes()->get<LoginMethod>(Attributes::keyLoginMethod.data());
    std::vector<std::unordered_map<std::string, std::string>> video_entries;

    if(loginMethod == LoginMethod::LaxNone) {
        data.insert("message", HttpViewData::htmlTranslate(req->getParameter("m")));
        respond(HttpResponse::newHttpViewResponse("login.csp", std::move(data)));
        return;
    }

    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    Attributes::userId_t userId = req->attributes()->get<Attributes::userId_t>(Attributes::userId.data());
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

    respond(HttpResponse::newHttpViewResponse("account.csp", std::move(data)));
}

void SlimyTube::PageController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();
    respond(HttpResponse::newHttpViewResponse("create.csp", std::move(data)));
}

void SlimyTube::PageController::upload(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& video_id) const {
    HttpViewData data = Settings::getInstance().getData();

    std::optional<Videos> video = RecordManager::getVideoIfExists(video_id);
    if(!video.has_value())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/account");

    data.insert("video_id", HttpViewData::htmlTranslate(video.value().getValueOfId()));
    data.insert("video_title", HttpViewData::htmlTranslate(video.value().getValueOfTitle()));
    respond(HttpResponse::newHttpViewResponse("upload.csp", std::move(data)));
}

void SlimyTube::PageController::error(std::function<void (const HttpResponsePtr &)> &&respond, HttpStatusCode err, const std::string& back) {
    HttpViewData data = Settings::getInstance().getData();
    data.insert("error-text", std::format("{} {}", static_cast<int>(err), drogon::statusCodeToString(err)));
    data.insert("back", back);
    respond(HttpResponse::newHttpViewResponse("error.csp", std::move(data)));
}
