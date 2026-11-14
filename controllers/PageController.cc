#include "Comments.h"
#include <controllers/RecordController.h>
#include <controllers/PageController.h>

#include <cstdint>
#include <drogon/orm/DbClient.h>
#include <models/utils/FormatUtility.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>
#include <drogon/utils/Utilities.h>
#include <lib/drogon/lib/src/HttpUtils.h>

#include <models/utils/ResolutionUtility.h>
#include <models/ViewCountManager.h>
#include <models/video/VideoTypes.h>
#include <models/RecordManager.h>
#include <models/Attributes.h>
#include <models/Settings.h>
#include <models/Users.h>
#include <models/Videos.h>

#include <unordered_map>
#include <string>
#include <utility>

using SlimyTube::Attributes::userId_t;
using SlimyTube::Attributes::LoginMethod;
using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;

using
    std::vector,
    std::string,
    std::unordered_map;

constexpr const std::string_view recommended_query = "SELECT v.*, u.name AS ch_name FROM videos AS v INNER JOIN users AS u ON v.channel = u.id WHERE state = $1 AND v.id != $2 ORDER BY RANDOM() LIMIT $3;";

namespace SlimyTube {
    inline void emplace_preview(const Row& row, vector<unordered_map<string, string>>& previews) {
        previews.emplace_back(unordered_map<string, string> {
                {"id", row[Videos::Cols::_id].as<string>() },
                {"length", FormatUtil::FormatVideoLength(row[Videos::Cols::_length].as<int64_t>()) },
                {"title", HttpViewData::htmlTranslate(row[Videos::Cols::_title].as<string>()) },
                {"channel", row["ch_name"].as<string>() },
                {"views", FormatUtil::FormatViewCount(row[Videos::Cols::_views].as<int64_t>()) },
                {"age", FormatUtil::FormatAge(row[Videos::Cols::_creation_time].as<int64_t>()) },
            });
    }

    inline void emplace_channel(const Row& row, vector<unordered_map<string, string>>& channels) {
        channels.emplace_back(unordered_map<string, string> {
                { "id", row[Users::Cols::_id].c_str() },
                { "name", row[Users::Cols::_name].c_str() },
                { "views", FormatUtil::FormatViewCount(row["total_views"].isNull() ? 0 : row["total_views"].as<int64_t>()) },
                { "videos", row["total_videos"].c_str() },
            });
    }


};

// TODO: Add some sort of smarter algorithm
vector<unordered_map<string, string>> SlimyTube::PageController::createPreviews(const string& ignore, size_t amount) const {
    DbClientPtr db = SlimyTube::RecordManager::getDbClient();
    Result queried_videos = db->execSqlSync(recommended_query.data(), static_cast<int32_t>(VideoState::Ready), ignore, amount);

    Mapper<Users> user_mapper = SlimyTube::RecordManager::getUserMapper();
    vector<unordered_map<string, string>> previews;
    for(const Row& video : queried_videos)
        emplace_preview(video, previews);

    return previews;
}

constexpr const std::string_view
    search_videos_query = "SELECT v.*, u.name AS ch_name FROM videos AS v INNER JOIN users AS u ON v.channel = u.id WHERE (v.title LIKE $1 OR v.description LIKE $1) AND v.state = $2 LIMIT $3 ORDER BY creation_time DESC;",
    search_users_query = "SELECT u.*, SUM(v.views) AS total_views, COUNT(v.id) AS total_videos FROM users AS u LEFT OUTER JOIN videos AS v ON u.id = v.channel AND v.state = $1 WHERE u.name LIKE $2 GROUP BY u.id LIMIT $3 ORDER BY name;";

size_t SlimyTube::PageController::createSearchResults(vector<unordered_map<string, string>>& previews, vector<unordered_map<string, string>>& channels, const string& query, size_t count) const {
    DbClientPtr db = RecordManager::getDbClient();
    const string query_w_wildcards = '%' + query + '%';

    const Result found_videos = db->execSqlSync(search_videos_query.data(), query_w_wildcards, static_cast<int32_t>(VideoState::Ready), count);
    for(const Row& video : found_videos)
        emplace_preview(video, previews);

    const Result found_users = db->execSqlSync(search_users_query.data(), static_cast<int32_t>(VideoState::Ready), query_w_wildcards, count);
    for(const Row& user : found_users)
        emplace_channel(user, channels);

    return found_videos.size() + found_users.size();
}

void SlimyTube::PageController::main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    vector<unordered_map<string, string>> previews = createPreviews("", 25);

    HttpViewData data = Settings::getInstance().getData();
    data.insert("previews", previews);

    respond(HttpResponse::newHttpViewResponse("main.csp", std::move(data)));
}

void SlimyTube::PageController::about(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const Settings& settings = Settings::getInstance();

    if(!settings.use_embeded_pages)
        return (void)respond(HttpResponse::newFileResponse("./static/about.html", "", drogon::CT_TEXT_HTML));

    HttpViewData data = settings.getData();
    return (void)respond(HttpResponse::newHttpViewResponse("about.csp", std::move(data)));
}

void SlimyTube::PageController::privacy(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const Settings& settings = Settings::getInstance();

    if(!settings.use_embeded_pages)
        return (void)respond(HttpResponse::newFileResponse("./static/privacy.html", "", drogon::CT_TEXT_HTML));

    HttpViewData data = settings.getData();
    return (void)respond(HttpResponse::newHttpViewResponse("privacy.csp", std::move(data)));
}

constexpr const std::string_view
    get_comments_query = "SELECT c.channel, c.creation_time, c.content, u.name AS ch_name FROM comments AS c INNER JOIN users AS u ON c.channel = u.id WHERE c.video = $1 ORDER BY creation_time DESC;";

namespace SlimyTube {
    inline void emplace_comment(const Row& row, vector<unordered_map<string, string>>& comments) {
        comments.emplace_back(unordered_map<string, string> {
                { "channel_id", row[Comments::Cols::_channel].c_str() },
                { "channel_name", row["ch_name"].c_str() },
                { "age", FormatUtil::FormatAge(row[Comments::Cols::_creation_time].as<int64_t>()) },
                { "text", HttpViewData::htmlTranslate(row[Comments::Cols::_content].c_str()) }
            });
    }
};

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = Settings::getInstance().getData();

    std::optional<Videos> video = RecordManager::getVideoIfExists(video_id);
    if(!video.has_value())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/");

    const Videos& video_ref = video.value();
    const Users& channel = video_ref.getUser(RecordManager::getDbClient());
    const string
        viewCountStr = FormatUtil::FormatViewCount(video_ref.getValueOfViews()),
        ageStr = FormatUtil::FormatAge(video_ref.getValueOfCreationTime()),
        title = HttpViewData::htmlTranslate(video_ref.getValueOfTitle()),
        desc = HttpViewData::htmlTranslate(video_ref.getValueOfDescription()),
        user_name = HttpViewData::htmlTranslate(channel.getValueOfName()),
        user_id = std::to_string(channel.getValueOfId());

    // find available resolution
    vector<uint16_t> resolutions = ResolutionUtility::GetResolutions(video_ref);
    vector<string> resolutions_str;
    for(uint16_t resolution : resolutions)
        resolutions_str.emplace_back(std::to_string(resolution));

    vector<unordered_map<string, string>> comments;
    DbClientPtr db = RecordManager::getDbClient();
    const Result found_comments = db->execSqlSync(get_comments_query.data(), video_id);
    for(const Row& comment : found_comments)
        emplace_comment(comment, comments);

    vector<unordered_map<string, string>> previews = createPreviews(video_id, 25);

    data.insert("video_id", video_id);
    data.insert("previews", previews);
    data.insert("comments", comments);
    data.insert("resolution", req->getParameter("quality"));
    data.insert("resolutions", resolutions_str);

    data.insert("video_views", viewCountStr);
    data.insert("video_age", ageStr);
    data.insert("video_title", title);
    data.insert("video_description", desc);
    data.insert("video_channel_name", user_name);
    data.insert("video_channel_id", user_id);
    data.insert("url", Settings::getInstance().url);

    respond(HttpResponse::newHttpViewResponse("watch.csp", std::move(data)));

    static ViewCountManager& mViewCountManager = ViewCountManager::getInstance();
    mViewCountManager.addView(video_id);
}

void SlimyTube::PageController::search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();
    vector<unordered_map<string, string>> previews, channels;

    const string& search_query = req->getParameter("q");
    size_t search_result_count = createSearchResults(previews, channels, search_query, 50);

    data.insert("result_count", search_result_count);
    data.insert("search_query", HttpViewData::htmlTranslate(search_query));
    data.insert("previews", previews);
    data.insert("channels", channels);
    data.insert("result_count", std::to_string(search_result_count));

    respond(HttpResponse::newHttpViewResponse("search.csp", std::move(data)));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();

    LoginMethod loginMethod = req->attributes()->get<LoginMethod>(Attributes::keyLoginMethod.data());
    vector<unordered_map<string, string>> video_entries;

    if(loginMethod == LoginMethod::LaxNone) {
        data.insert("message", HttpViewData::htmlTranslate(req->getParameter("m")));
        respond(HttpResponse::newHttpViewResponse("login.csp", std::move(data)));
        return;
    }

    DbClientPtr db = drogon::app().getDbClient();
    Mapper<Users> user_mapper(db);

    Attributes::userId_t userId = req->attributes()->get<Attributes::userId_t>(Attributes::userId.data());
    const Users user = user_mapper.findByPrimaryKey(userId);
    const vector<Videos> videos = user.getVideos(db);

    for(const Videos& video : videos) {
        video_entries.emplace_back(unordered_map<string, string>{
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

void SlimyTube::PageController::reg(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)>&& respond) const {
    HttpViewData data = Settings::getInstance().getData();
    respond(HttpResponse::newHttpViewResponse("reg.csp", std::move(data)));
}

void SlimyTube::PageController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = Settings::getInstance().getData();
    respond(HttpResponse::newHttpViewResponse("create.csp", std::move(data)));
}

void SlimyTube::PageController::upload(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = Settings::getInstance().getData();

    std::optional<Videos> video = RecordManager::getVideoIfExists(video_id);
    if(!video.has_value())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/account");

    const Videos& video_ref = video.value();
    userId_t userId = req->attributes()->get<userId_t>(SlimyTube::Attributes::userId.data());
    if(video_ref.getValueOfChannel() != userId) // video doesn't belong to the user
        return(void) error(std::move(respond), HttpStatusCode::k403Forbidden, "/account");

    data.insert("video_id", HttpViewData::htmlTranslate(video_ref.getValueOfId()));
    data.insert("video_title", HttpViewData::htmlTranslate(video_ref.getValueOfTitle()));
    respond(HttpResponse::newHttpViewResponse("upload.csp", std::move(data)));
}

constexpr const std::string_view get_channel_query = "SELECT u.*, SUM(v.views) AS total_views, COUNT(v.id) AS total_videos FROM users AS u LEFT OUTER JOIN videos AS v ON u.id = v.channel AND v.state = $1 WHERE u.id = $2 GROUP BY u.id;";

void SlimyTube::PageController::channel(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const uint64_t& user_id) const {
    HttpViewData data = Settings::getInstance().getData();

    DbClientPtr db = drogon::app().getDbClient();
    Mapper<Users> user_mapper(db);
    Mapper<Videos> video_mapper(db);

    const Result users = db->execSqlSync(get_channel_query.data(), static_cast<int32_t>(VideoState::Ready), user_id);
    const Row& user = users[0];
    if(users.empty())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/");

    vector<Videos> videos = video_mapper
        .orderBy(Videos::Cols::_creation_time, SortOrder::DESC)
        .findBy(
            Criteria(Videos::Cols::_channel, CompareOperator::EQ, user_id) &&
            Criteria(Videos::Cols::_state, CompareOperator::EQ, static_cast<int32_t>(VideoState::Ready)));

    vector<unordered_map<string, string>> previews;
    for(const Videos& video : videos) {
        previews.emplace_back(unordered_map<string, string> {
                {"id", video.getValueOfId()},
                {"length", FormatUtil::FormatVideoLength(video.getValueOfLength())},
                {"title", HttpViewData::htmlTranslate(video.getValueOfTitle())},
                {"views", FormatUtil::FormatViewCount(video.getValueOfViews()) },
                {"age", FormatUtil::FormatAge(video.getValueOfCreationTime()) },
            });
    }

    data.insert("url", Settings::getInstance().url);
    data.insert("channel_name", user[Users::Cols::_name].c_str());
    data.insert("channel_views", user["total_views"].isNull() ? "0" : user["total_views"].c_str());
    data.insert("channel_videos", user["total_videos"].c_str());
    data.insert("previews", previews);

    respond(HttpResponse::newHttpViewResponse("channel.csp", std::move(data)));
}

void SlimyTube::PageController::edit(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = Settings::getInstance().getData();

    std::optional<Videos> video = RecordManager::getVideoIfExists(video_id);
    if(!video.has_value())
        return(void) error(std::move(respond), HttpStatusCode::k404NotFound, "/account");

    const Videos& video_ref = video.value();
    userId_t userId = req->attributes()->get<userId_t>(SlimyTube::Attributes::userId.data());
    if(video_ref.getValueOfChannel() != userId) // video doesn't belong to the user
        return(void) error(std::move(respond), HttpStatusCode::k403Forbidden, "/account");

    const Users& channel = video_ref.getUser(RecordManager::getDbClient());
    const string
        viewCountStr = FormatUtil::FormatViewCount(video_ref.getValueOfViews()),
        ageStr = FormatUtil::FormatAge(video_ref.getValueOfCreationTime()),
        title = HttpViewData::htmlTranslate(video_ref.getValueOfTitle()),
        desc = HttpViewData::htmlTranslate(video_ref.getValueOfDescription()),
        user_name = HttpViewData::htmlTranslate(channel.getValueOfName()),
        user_id = std::to_string(channel.getValueOfId());

    data.insert("video_id", video_id);

    data.insert("video_views", viewCountStr);
    data.insert("video_age", ageStr);
    data.insert("video_title", title);
    data.insert("video_description", desc);
    data.insert("video_channel_name", user_name);

    respond(HttpResponse::newHttpViewResponse("edit.csp", std::move(data)));
}

HttpResponsePtr SlimyTube::PageController::errorResponse(HttpStatusCode err, const string& back) {
    HttpViewData data = Settings::getInstance().getData();
    data.insert("error-text", std::format("{} {}", static_cast<int>(err), drogon::statusCodeToString(err)));
    data.insert("back", back);
    HttpResponsePtr res = HttpResponse::newHttpViewResponse("error.csp", std::move(data));
    res->setStatusCode(err);
    return res;
}

void SlimyTube::PageController::error(std::function<void (const HttpResponsePtr &)> &&respond, HttpStatusCode err, const string& back) {
    respond(errorResponse(err, back));
}
