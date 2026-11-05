#include <controllers/RecordController.h>
#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>

#include <models/RecordManager.h>
#include <models/Attributes.h>
#include <models/Users.h>
#include <models/Videos.h>
#include <models/utils/TimeUtility.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

using namespace drogon;
using namespace drogon::orm;
using SlimyTube::Attributes::userId_t;
using namespace drogon_model::SlimyTube;

#define api_ui_error(code, back)                                        \
    return (void)respond(api                                            \
        ? HttpResponse::newHttpResponse(code, ContentType::CT_NONE)     \
        : PageController::errorResponse(code, back))

#define api_redirect_error(code, loc_) {                                \
        if(api) {                                                       \
            return (void)respond(HttpResponse::newHttpResponse(code, ContentType::CT_NONE)); \
        } else {                                                        \
            HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE); \
            res->addHeader(Headers::location.data(), loc_);             \
            return (void)respond(res);                                  \
        }                                                               \
    }

void SlimyTube::RecordController::userRegister(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    Mapper<Users> user_mapper = RecordManager::getUserMapper();

    const std::string&
        name = req->getParameter(Query::username.data()),
        pass = req->getParameter(Query::password.data());

    // details not provided
    if(name.empty() || pass.empty()) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, ContentType::CT_NONE));
        return;
    }

    // username already taken
    if(RecordManager::nameExists(name, user_mapper)) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k409Conflict, ContentType::CT_NONE));
        return;
    }

    std::vector<uint8_t> salt = RecordManager::createSalt();

    uint8_t hash[32];
    RecordManager::createHash(pass, salt, hash);

    Users user;
    user.setName(name);
    user.setHash(std::string(reinterpret_cast<char*>(hash), sizeof(hash)));
    user.setSalt(std::string(reinterpret_cast<const char*>(salt.data()), salt.size()));

    user_mapper.insert(user);

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader(Headers::location.data(), "/account?m=Account%20created%20successfully");
    respond(res);
}

void SlimyTube::RecordController::userLogin(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    const Attributes::LoginMethod& loginMethod = req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data());

    if(loginMethod == Attributes::LoginMethod::LaxNone)
        api_redirect_error(HttpStatusCode::k401Unauthorized, "/account?m=Wrong%20name%20or%20password");

    if(loginMethod != Attributes::LoginMethod::Session)
        req->session()->insert(Session::userId.data(), req->attributes()->get<userId_t>(Attributes::userId.data()));

    api_redirect_error(HttpStatusCode::k200OK, "/account");
}

void SlimyTube::RecordController::userLogout(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    if(req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data()) != Attributes::LoginMethod::Session)
        api_ui_error(HttpStatusCode::k400BadRequest, "/account");

    req->session()->clear();

    api_redirect_error(HttpStatusCode::k200OK, "/account?m=Logged%20out%20successfully");
}

void SlimyTube::RecordController::userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    DbClientPtr db = RecordManager::getDbClient();
    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();

    userId_t userId = req->attributes()->get<userId_t>(Attributes::userId.data());
    Users user = user_mapper.findByPrimaryKey(userId);

    // Delete all videos made by the account
    const std::vector<Videos> videos = user.getVideos(db);
    for(const Videos& video : videos)
        RecordManager::deleteVideo(video);

    user_mapper.deleteByPrimaryKey(req->attributes()->get<userId_t>(Attributes::userId.data()));
    req->session()->clear();

    api_redirect_error(HttpStatusCode::k200OK, "/account?m=Account%20deleted%20successfully");
}

void SlimyTube::RecordController::userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    const std::string&
        new_name = req->getParameter(Query::newUsername.data());

    if(new_name.empty())
        api_ui_error(HttpStatusCode::k400BadRequest, "/account");

    if(RecordManager::nameExists(new_name, user_mapper))
        api_ui_error(HttpStatusCode::k409Conflict, "/account");

    userId_t userId = req->getAttributes()->get<userId_t>(Attributes::userId.data());
    Criteria crit = Criteria(Users::Cols::_id, CompareOperator::EQ, userId);
    user_mapper.updateBy({ Users::Cols::_name }, crit, new_name);

    api_redirect_error(HttpStatusCode::k200OK, "/account");
}

void SlimyTube::RecordController::userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    const std::string&
        new_pass = req->getParameter(Query::newPassword.data());

    if(new_pass.empty())
        api_ui_error(HttpStatusCode::k400BadRequest, "/account");

    std::vector<uint8_t> salt = RecordManager::createSalt();

    uint8_t hash[32];
    RecordManager::createHash(new_pass, salt, hash);

    userId_t userId = req->getAttributes()->get<userId_t>(Attributes::userId.data());
    Criteria crit = Criteria(Users::Cols::_id, CompareOperator::EQ, userId);
    Users user = user_mapper.findByPrimaryKey(userId);
    user.setHash(std::string(reinterpret_cast<const char*>(hash), sizeof(hash)));
    user.setSalt(std::string(reinterpret_cast<const char*>(salt.data()), salt.size()));
    user_mapper.update(user);

    req->session()->clear();

    api_redirect_error(HttpStatusCode::k200OK, "/account?m=Changed%20password%20successfully")
}

void SlimyTube::RecordController::videoCreate(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    const std::string&
        title = req->getParameter(Query::videoTitle.data()),
        desc = req->getParameter(Query::videoDescription.data());

    if(title.empty())
        api_ui_error(HttpStatusCode::k400BadRequest, "/account");

    const userId_t
        channel = req->attributes()->get<userId_t>(Attributes::userId.data());

    std::string id = RecordManager::createVideo(channel, title, desc);

    api_redirect_error(HttpStatusCode::k200OK, "/video/upload/" + id)
}

void SlimyTube::RecordController::videoDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    const userId_t channel = req->attributes()->get<userId_t>(Attributes::userId.data());
    std::optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value())
        api_ui_error(HttpStatusCode::k404NotFound, "/account");

    if(video.value().getValueOfChannel() != channel)
        api_ui_error(HttpStatusCode::k403Forbidden, "/account");

    RecordManager::deleteVideo(video.value());

    api_redirect_error(HttpStatusCode::k200OK, "/account");
}

void SlimyTube::RecordController::videoChangeTitle(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    const userId_t channel = req->attributes()->get<userId_t>(Attributes::userId.data());
    std::optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value())
        api_ui_error(HttpStatusCode::k404NotFound, "/account");

    if(video.value().getValueOfChannel() != channel)
        api_ui_error(HttpStatusCode::k403Forbidden, "/account");

    const std::string& title = req->getParameter(Query::videoTitle.data());
    if(title.empty())
        api_ui_error(HttpStatusCode::k400BadRequest, "/video/edit/" + id);

    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();
    video_mapper.updateBy({ Videos::Cols::_title }, Criteria(Videos::Cols::_id, CompareOperator::EQ, id), title);

    api_redirect_error(HttpStatusCode::k200OK, "/video/edit/" + id);
}

void SlimyTube::RecordController::videoChangeDescription(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const {
    const bool& api = req->attributes()->get<bool>(Attributes::apiUsed.data());
    const userId_t channel = req->attributes()->get<userId_t>(Attributes::userId.data());
    std::optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value())
        api_ui_error(HttpStatusCode::k404NotFound, "/account");

    if(video.value().getValueOfChannel() != channel)
        api_ui_error(HttpStatusCode::k403Forbidden, "/account");

    const std::string& description = req->getParameter(Query::videoDescription.data());

    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();
    video_mapper.updateBy({ Videos::Cols::_description }, Criteria(Videos::Cols::_id, CompareOperator::EQ, id), description);

    api_redirect_error(HttpStatusCode::k200OK, "/video/edit/" + id);
}
