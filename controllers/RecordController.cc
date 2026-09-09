#include <Users.h>
#include <controllers/RecordController.h>

using namespace drogon_model::SlimyTube;

void SlimyTube::RecordController::registerUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    Users user;
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);
}
void SlimyTube::RecordController::deleteUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);
    user_mapper.deleteByPrimaryKey(req->getAttributes()->get<uint32_t>("userId"));
    req->session()->erase("userId");
}
