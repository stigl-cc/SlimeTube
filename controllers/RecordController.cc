#include <controllers/RecordController.h>
#include <controllers/PageController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>

#include <models/RecordManager.h>
#include <models/Attributes.hh>
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
    respond(HttpResponse::newHttpResponse(HttpStatusCode::k201Created, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userLogin(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    Attributes::LoginMethod loginMethod = req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data());
    if(loginMethod == Attributes::LoginMethod::LaxNone) {
        HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
        res->addHeader(Headers::location.data(), "/account?m=Wrong%20name%20or%20password");
        respond(res);
        return;
    }

    if(loginMethod != Attributes::LoginMethod::Session) {
        req->session()->insert(Session::userId.data(), req->attributes()->get<userId_t>(Attributes::userId.data()));
    }

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader(Headers::location.data(), "/account");
    respond(res);
}

void SlimyTube::RecordController::userLogout(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    if(req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data()) != Attributes::LoginMethod::Session) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, ContentType::CT_NONE));
        return;
    }

    req->session()->clear();

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader(Headers::location.data(), "/account?m=Logged%20out%20successfully");
    respond(res);
}

void SlimyTube::RecordController::userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
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

    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    const std::string&
        new_name = req->getParameter(Query::newUsername.data());

    if(new_name.empty()) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, ContentType::CT_NONE));
        return;
    }

    if(RecordManager::nameExists(new_name, user_mapper)) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k409Conflict, ContentType::CT_NONE));
        return;
    }

    userId_t userId = req->getAttributes()->get<userId_t>(Attributes::userId.data());
    Criteria crit = Criteria(Users::Cols::_id, CompareOperator::EQ, userId);
    user_mapper.updateBy({ Users::Cols::_name }, crit, new_name);
    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    const std::string&
        new_pass = req->getParameter(Query::newPassword.data());

    if(new_pass.empty())
        return(void) respond(HttpResponse::newHttpResponse(drogon::k400BadRequest, drogon::CT_NONE));

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

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader(Headers::location.data(), "/account?m=Changed%20password%20successfully");
    respond(res);
}

void SlimyTube::RecordController::videoCreate(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    const std::string&
        title = req->getParameter(Query::videoTitle.data()),
        desc = req->getParameter(Query::videoDescription.data());

    if(title.empty())
        return(void) PageController::error(std::move(respond), HttpStatusCode::k400BadRequest, "/account");

    const uint64_t
        channel = req->attributes()->get<userId_t>(Attributes::userId.data());

    std::string id = RecordManager::createVideo(channel, title, desc);

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader(Headers::location.data(), "/upload/" + id);
    respond(res);
}
