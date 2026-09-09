#include <drogon/HttpAppFramework.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <filters/Authenticator.h>

#include <models/Users.h>
#include <models/Videos.h>

using namespace drogon_model::SlimyTube;

namespace orm = drogon::orm;

void SlimyTube::Authenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    if(req->session()->find("id")) {
        req->session()->get<uint64_t>("id");
        orm::DbClientPtr db = drogon::app().getDbClient("default");
        drogon::orm::Mapper<Users> user_mapper(db);
        user_mapper.findOne(drogon::orm::Criteria(Users::Cols::_id, drogon::orm::CompareOperator::EQ));
    } else {

    }
}
