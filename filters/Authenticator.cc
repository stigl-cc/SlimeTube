#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <filters/Authenticator.h>

#include <models/Users.h>
#include <models/Videos.h>

using namespace drogon_model::SlimyTube;
namespace orm = drogon::orm;

constexpr const std::string_view userIdSessionKey = "userId";

void SlimyTube::Authenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    orm::DbClientPtr db = drogon::app().getDbClient();
    drogon::orm::Mapper<Users> user_mapper(db);

    if(req->session()->find(userIdSessionKey.data())) {
        uint32_t userId = req->session()->get<uint32_t>(userIdSessionKey.data());

        req->attributes()->insert("userId", userId);

        //Users u = user_mapper.findByPrimaryKey(userId);
        //std::cout << u.getValueOfName() << "\n";
        mcb(drogon::HttpResponse::newHttpResponse(static_cast<drogon::HttpStatusCode>(userId), drogon::ContentType::CT_NONE));
    } else {
        req->session()->insert(userIdSessionKey.data(), uint32_t(12));
        mcb(drogon::HttpResponse::newHttpResponse(drogon::k401Unauthorized, drogon::ContentType::CT_NONE));
    }
}
