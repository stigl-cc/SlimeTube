#include <controllers/RecordController.h>
#include <controllers/PageController.h>

#include <drogon/orm/DbClient.h>
#include <models/utils/FormatUtility.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>
#include <drogon/utils/Utilities.h>
#include <lib/drogon/lib/src/HttpUtils.h>

#include <models/RecordManager.h>
#include <models/Attributes.hh>
#include <models/Settings.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <unordered_map>
#include <string>
#include <utility>

using SlimyTube::Attributes::LoginMethod;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;

constexpr const std::string_view recommended_query = "SELECT * FROM videos WHERE state = $1 AND id != $2 ORDER BY RANDOM() LIMIT $3;";

// TODO: Add some sort of smarter algorithm
std::vector<std::unordered_map<std::string, std::string>> SlimyTube::PageController::createPreviews(const std::string& ignore, size_t amount) const {
    DbClientPtr db = SlimyTube::RecordManager::getDbClient();
    Result queriedVideos = db->execSqlSync(recommended_query.data(), static_cast<int32_t>(VideoState::Ready), ignore, amount);

    Mapper<Users> user_mapper = SlimyTube::RecordManager::getUserMapper();
    std::vector<Videos> videos;
    for(const Row& video : queriedVideos)
        videos.emplace_back(video);

    std::vector<std::unordered_map<std::string, std::string>> previews;
    for(const Videos& video : videos) {
        std::string viewCountStr = FormatUtil::FormatViewCount(video.getValueOfViews());
        std::string ageStr = FormatUtil::FormatVideoAge(video.getValueOfCreationTime());

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
    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews("", 25);

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
        viewCountStr = FormatUtil::FormatViewCount(video.value().getValueOfViews()),
        ageStr = FormatUtil::FormatVideoAge(video.value().getValueOfCreationTime()),
        title = HttpViewData::htmlTranslate(video.value().getValueOfTitle()),
        desc = HttpViewData::htmlTranslate(video.value().getValueOfDescription()),
        user_name = HttpViewData::htmlTranslate(channel.getValueOfName()),
        user_id = std::to_string(channel.getValueOfId());

    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews(video_id, 25);

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

    std::vector<std::unordered_map<std::string, std::string>> previews = createPreviews("", 25);

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
                { "state", std::to_string(video.getValueOfState()) },
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
    HttpResponsePtr res = HttpResponse::newHttpViewResponse("error.csp", std::move(data));
    res->setStatusCode(err);
    respond(res);
}
