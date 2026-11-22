#include <controllers/PageController.h>
#include <controllers/RecordController.h>

#include <cstdint>
#include <drogon/orm/DbClient.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpViewData.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>
#include <drogon/utils/Utilities.h>
#include <lib/drogon/lib/src/HttpUtils.h>

#include <models/utils/FormatUtility.h>
#include <models/utils/ResolutionUtility.h>
#include <models/ViewCountManager.h>
#include <models/video/VideoTypes.h>
#include <models/RecordManager.h>
#include <models/Attributes.h>
#include <models/Settings.h>
#include <models/Users.h>
#include <models/Videos.h>
#include <models/Comments.h>
#include <models/Preview.h>

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

namespace SlimyTube {
    constexpr const std::string_view
        recommended_query   = "SELECT v.id, v.length, v.title, u.name, v.views, v.creation_time FROM videos AS v INNER JOIN users AS u ON v.channel = u.id WHERE v.state = $1 AND v.id != $2 ORDER BY RANDOM() LIMIT $3;",
        search_videos_query = "SELECT v.id, v.length, v.title, u.name, v.views, v.creation_time FROM videos AS v INNER JOIN users AS u ON v.channel = u.id WHERE v.state = $1 AND (v.title LIKE $2 OR v.description LIKE $2) ORDER BY creation_time DESC LIMIT $3;";

    inline void emplace_preview(const Row& row, vector<Preview>& previews) {
        previews.emplace_back(
            row[0].as<string>(),
            FormatUtil::FormatVideoLength(row[1].as<int64_t>()),
            HttpViewData::htmlTranslate(row[2].as<string>()),
            row[3].as<string>(),
            FormatUtil::FormatViewCount(row[4].as<int64_t>()),
            FormatUtil::FormatAge(row[5].as<int64_t>())
        );
    }

    constexpr const std::string_view search_users_query = "SELECT u.id, u.name, SUM(v.views), COUNT(v.id) FROM users AS u LEFT OUTER JOIN videos AS v ON u.id = v.channel AND v.state = $1 WHERE u.name LIKE $2 GROUP BY u.id ORDER BY name LIMIT $3;";
    inline void emplace_channel(const Row& row, vector<Channel>& channels) {
        channels.emplace_back(
            row[0].as<int64_t>(),
            row[1].as<string>(),
            FormatUtil::FormatViewCount(row[2].isNull() ? 0 : row[2].as<int64_t>()),
            row[3].as<int64_t>()
        );
    }

    constexpr const std::string_view get_comments_query = "SELECT c.channel, c.creation_time, c.content, u.name FROM comments AS c LEFT OUTER JOIN users AS u ON c.channel = u.id WHERE c.video = $1 ORDER BY creation_time DESC;";
    inline void emplace_comment(const Row& row, vector<Comment>& comments) {
        const Field& ch_name = row[3];

        comments.emplace_back(
            row[0].as<int64_t>(),
            ch_name.isNull() ? "Anonymous" : ch_name.as<string>(),
            FormatUtil::FormatAge(row[1].as<int64_t>()),
            HttpViewData::htmlTranslate(row[2].as<string>())
        );
    }
};

// TODO: Add some sort of smarter algorithm
size_t SlimyTube::PageController::createPreviews(const string& ignore, size_t amount, vector<Preview>& previews) const {
    DbClientPtr db = SlimyTube::RecordManager::getDbClient();
    Result queried_videos = db->execSqlSync(recommended_query.data(), static_cast<int32_t>(VideoState::Ready), ignore, amount);

    previews.reserve(queried_videos.size());
    for(const Row& video : queried_videos)
        emplace_preview(video, previews);

    return queried_videos.size();
}

size_t SlimyTube::PageController::createSearchResults(vector<Preview>& previews, vector<Channel>& channels, const string& query, size_t count) const {
    DbClientPtr db = RecordManager::getDbClient();
    const string query_w_wildcards = '%' + query + '%';

    const Result found_users = db->execSqlSync(search_users_query.data(), static_cast<int32_t>(VideoState::Ready), query_w_wildcards, count);
    channels.reserve(found_users.size());
    for(const Row& user : found_users)
        emplace_channel(user, channels);

    const Result found_videos = db->execSqlSync(search_videos_query.data(), static_cast<int32_t>(VideoState::Ready), query_w_wildcards, count - found_users.size());
    previews.reserve(found_videos.size());
    for(const Row& video : found_videos)
        emplace_preview(video, previews);

    return found_videos.size() + found_users.size();
}

size_t SlimyTube::PageController::getCommentsForVideo(const std::string& videoId, vector<Comment>& comments) {
    DbClientPtr db = RecordManager::getDbClient();

    const Result found_comments = db->execSqlSync(get_comments_query.data(), videoId);
    comments.reserve(found_comments.size());
    for(const Row& comment : found_comments)
        emplace_comment(comment, comments);

    return found_comments.size();
}

void SlimyTube::PageController::main(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    std::vector<Preview> previews;
    createPreviews("", mSettings.limits.max_recommended_previews_main, previews);

    HttpViewData data = mSettings.getData();
    data.insert("previews", std::move(previews));

    respond(HttpResponse::newHttpViewResponse("main.csp", std::move(data)));
}

void SlimyTube::PageController::about(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    if(!mSettings.use_embeded_pages)
        return (void)respond(HttpResponse::newFileResponse("./static/about.html", "", drogon::CT_TEXT_HTML));

    HttpViewData data = mSettings.getData();
    return (void)respond(HttpResponse::newHttpViewResponse("about.csp", std::move(data)));
}

void SlimyTube::PageController::privacy(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    if(!mSettings.use_embeded_pages)
        return (void)respond(HttpResponse::newFileResponse("./static/privacy.html", "", drogon::CT_TEXT_HTML));

    HttpViewData data = mSettings.getData();
    return (void)respond(HttpResponse::newHttpViewResponse("privacy.csp", std::move(data)));
}

void SlimyTube::PageController::watch(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = mSettings.getData();

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

    vector<Comment> comments;
    DbClientPtr db = RecordManager::getDbClient();
    const Result found_comments = db->execSqlSync(get_comments_query.data(), video_id);
    for(const Row& comment : found_comments)
        emplace_comment(comment, comments);

    vector<Preview> previews;
    createPreviews(video_id, mSettings.limits.max_recommended_previews_watch, previews);

    data.insert("video_id", video_id);
    data.insert("previews", std::move(previews));
    data.insert("comments", std::move(comments));
    data.insert("resolution", req->getParameter("quality"));
    data.insert("resolutions", std::move(resolutions_str));

    data.insert("video_views", viewCountStr);
    data.insert("video_age", ageStr);
    data.insert("video_title", title);
    data.insert("video_description", desc);
    data.insert("video_channel_name", user_name);
    data.insert("video_channel_id", user_id);
    data.insert("url", mSettings.url);

    respond(HttpResponse::newHttpViewResponse("watch.csp", std::move(data)));
}

void SlimyTube::PageController::search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = mSettings.getData();
    vector<Preview> previews;
    vector<Channel> channels;

    const string& search_query = req->getParameter("q");
    size_t search_result_count = createSearchResults(previews, channels, search_query, mSettings.limits.max_search_results);

    data.insert("result_count", search_result_count);
    data.insert("search_query", HttpViewData::htmlTranslate(search_query));
    data.insert("previews", std::move(previews));
    data.insert("channels", std::move(channels));
    data.insert("result_count", std::to_string(search_result_count));

    respond(HttpResponse::newHttpViewResponse("search.csp", std::move(data)));
}

void SlimyTube::PageController::account(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = mSettings.getData();

    LoginMethod loginMethod = req->attributes()->get<LoginMethod>(Attributes::keyLoginMethod.data());

    if(loginMethod == LoginMethod::LaxNone) {
        data.insert("register_btn", mSettings.allow_user_creation);
        data.insert("message", HttpViewData::htmlTranslate(req->getParameter("m")));
        respond(HttpResponse::newHttpViewResponse("login.csp", std::move(data)));
        return;
    }

    DbClientPtr db = drogon::app().getDbClient();
    Mapper<Users> user_mapper(db);
    Mapper<Videos> video_mapper(db);

    Attributes::userId_t userId = req->attributes()->get<Attributes::userId_t>(Attributes::userId.data());
    const Users user = user_mapper.findByPrimaryKey(userId);
    const vector<Videos> videos =  video_mapper.orderBy(Videos::Cols::_creation_time, SortOrder::DESC).findBy(Criteria(Videos::Cols::_channel, CompareOperator::EQ, userId));

    vector<unordered_map<string, string>> video_entries;
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
    data.insert("videos", std::move(video_entries));

    respond(HttpResponse::newHttpViewResponse("account.csp", std::move(data)));
}

void SlimyTube::PageController::reg(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)>&& respond) const {
    if(!mSettings.allow_user_creation)
        return(void) error(std::move(respond), HttpStatusCode::k403Forbidden, "/account?m=User%20creation%20not%20allowed");

    HttpViewData data = mSettings.getData();
    respond(HttpResponse::newHttpViewResponse("reg.csp", std::move(data)));
}

void SlimyTube::PageController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    HttpViewData data = mSettings.getData();
    respond(HttpResponse::newHttpViewResponse("create.csp", std::move(data)));
}

void SlimyTube::PageController::upload(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = mSettings.getData();

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
    HttpViewData data = mSettings.getData();

    DbClientPtr db = drogon::app().getDbClient();
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

    vector<Preview> previews;
    previews.reserve(videos.size());

    for(const Videos& video : videos) {
        previews.emplace_back(
            video.getValueOfId(),
            FormatUtil::FormatVideoLength(video.getValueOfLength()),
            HttpViewData::htmlTranslate(video.getValueOfTitle()),
            "", // channel - not neceseary for this
            FormatUtil::FormatViewCount(video.getValueOfViews()),
            FormatUtil::FormatAge(video.getValueOfCreationTime())
        );
    }

    data.insert("url", mSettings.url);
    data.insert("channel_name", user[Users::Cols::_name].c_str());
    data.insert("channel_views", user["total_views"].isNull() ? "0" : user["total_views"].c_str());
    data.insert("channel_videos", user["total_videos"].c_str());
    data.insert("previews", std::move(previews));

    respond(HttpResponse::newHttpViewResponse("channel.csp", std::move(data)));
}

void SlimyTube::PageController::edit(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const string& video_id) const {
    HttpViewData data = mSettings.getData();

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

SlimyTube::PageController::PageController() : mSettings(Settings::getInstance()) { }
