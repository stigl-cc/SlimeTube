#include <controllers/RecordController.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>

#include <models/Attributes.hh>
#include <models/Session.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

using namespace drogon;
using SlimyTube::Session::keyUserId;
using namespace drogon_model::SlimyTube;

void SlimyTube::RecordController::userRegister(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    std::string
        name = req->getParameter("name"),
        pass = req->getParameter("pass");

    // details not provided
    if(name.empty() || pass.empty()) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, ContentType::CT_NONE));
        return;
    }

    // username already taken
    if(!user_mapper.findBy(orm::Criteria(Users::Cols::_name, orm::CompareOperator::EQ, name)).empty()) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k409Conflict, ContentType::CT_NONE));
        return;
    }

    uint8_t hash[32], salt[16];

    if(!RAND_bytes(salt, sizeof(salt))) {
        LOG_WARN << "RAND_bytes Failed";
        ERR_print_errors_fp(stderr);

        respond(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, ContentType::CT_NONE));
        return;
    }

    if(!PKCS5_PBKDF2_HMAC(
            pass.data(), pass.size(),
            salt, sizeof(salt),
            1024, EVP_sha256(),
            sizeof(hash), hash)) {
        LOG_WARN << "PBKDF2 Failed";
        ERR_print_errors_fp(stderr);

        respond(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, ContentType::CT_NONE));
        return;
    }

    Users user;
    user.setName(name);
    user.setSalt(std::string(reinterpret_cast<char*>(salt), sizeof(salt)));
    user.setHash(std::string(reinterpret_cast<char*>(hash), sizeof(hash)));

    user_mapper.insert(user);
    respond(HttpResponse::newHttpResponse(HttpStatusCode::k201Created, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userLogin(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    Attributes::LoginMethod loginMethod = req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data());
    if(loginMethod == Attributes::LoginMethod::LaxNone) {
        HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
        res->addHeader("Location", "/account?m=Wrong%20name%20or%20password");
        respond(res);
        return;
    }

    if(loginMethod != Attributes::LoginMethod::Session) {
        req->session()->insert(Session::keyUserId.data(), req->attributes()->get<uint32_t>(Attributes::keyUserId.data()));
    }

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader("Location", "/account");
    respond(res);
}

void SlimyTube::RecordController::userLogout(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    if(req->attributes()->get<Attributes::LoginMethod>(Attributes::keyLoginMethod.data()) != Attributes::LoginMethod::Session) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, ContentType::CT_NONE));
        return;
    }

    req->session()->clear();

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k303SeeOther, ContentType::CT_NONE);
    res->addHeader("Location", "/account?m=Logged%20out%20successfully");
    respond(res);
}

void SlimyTube::RecordController::userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    uint32_t userId = req->attributes()->get<uint32_t>(Attributes::keyUserId.data());
    Users user = user_mapper.findByPrimaryKey(userId);

    // Delete all videos made by the account
    drogon::orm::Mapper<Videos> video_mapper(db);
    const std::vector<Videos> videos = user.getVideos(db);
    for(const Videos& video : videos)
        video_mapper.deleteByPrimaryKey(video.getPrimaryKey());

    user_mapper.deleteByPrimaryKey(req->getAttributes()->get<uint32_t>(keyUserId.data()));
    req->session()->clear();

    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    // TODO: Check if username is in conflict
    uint32_t userId = req->getAttributes()->get<uint32_t>(Attributes::keyUserId.data());
    orm::Criteria crit = orm::Criteria(Users::Cols::_id, orm::CompareOperator::EQ, userId);
    user_mapper.updateBy({ "name" }, crit, req->getParameter("new_name"));
    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    // TODO: Fix to change the hash instead
    uint32_t userId = req->getAttributes()->get<uint32_t>(Attributes::keyUserId.data());
    orm::Criteria crit = orm::Criteria(Users::Cols::_id, userId);
    user_mapper.updateBy({ "pass" }, crit, req->getParameter("new_pass"));
    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}
