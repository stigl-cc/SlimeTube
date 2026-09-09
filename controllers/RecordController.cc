#include <controllers/RecordController.h>

#include <Session.hh>
#include <Users.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

using namespace drogon;
using SlimyTube::Session::keyUserId;
using namespace drogon_model::SlimyTube;

void SlimyTube::RecordController::registerUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    Users user;
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);
}

void SlimyTube::RecordController::deleteUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    user_mapper.deleteByPrimaryKey(req->getAttributes()->get<uint32_t>(keyUserId.data()));
    req->session()->erase(keyUserId.data());

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k200OK, ContentType::CT_NONE);
    callback(res);
}
