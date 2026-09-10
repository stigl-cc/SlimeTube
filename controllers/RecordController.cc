#include <controllers/RecordController.h>

#include <Session.hh>
#include <Users.h>
#include <Videos.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
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

    // add salt to password string (to be hashed)
    pass.append(salt, salt + sizeof(salt));

    if(SHA256(reinterpret_cast<const uint8_t*>(pass.c_str()), pass.size(), hash) == nullptr) {
        LOG_WARN << "SHA256 Failed";
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

void SlimyTube::RecordController::userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    uint32_t userId = req->attributes()->get<uint32_t>("userId");
    Users user = user_mapper.findByPrimaryKey(userId);

    // Delete all videos made by the account
    drogon::orm::Mapper<Videos> video_mapper(db);
    std::vector<Videos> videos = user.getVideos(db);
    for(const Videos& video : videos)
        video_mapper.deleteByPrimaryKey(video.getPrimaryKey());

    user_mapper.deleteByPrimaryKey(req->getAttributes()->get<uint32_t>(keyUserId.data()));
    req->session()->erase(keyUserId.data());

    respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE));
}

void SlimyTube::RecordController::userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {

}

void SlimyTube::RecordController::userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const {

}
