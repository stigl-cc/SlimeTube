#include <Session.hh>
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
using drogon::HttpStatusCode,
    drogon::ContentType,
    drogon::HttpResponse;

void SlimyTube::Authenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    orm::DbClientPtr db = drogon::app().getDbClient();
    orm::Mapper<Users> user_mapper(db);

    if(req->session()->find(Session::keyUserId.data())) {
        uint32_t userId = req->session()->get<uint32_t>(Session::keyUserId.data());

        req->attributes()->insert("userId", userId);

        //Users u = user_mapper.findByPrimaryKey(userId);
        //std::cout << u.getValueOfName() << "\n";
        mcb(HttpResponse::newHttpResponse(static_cast<HttpStatusCode>(userId), ContentType::CT_NONE));
    } else {
        req->session()->insert(Session::keyUserId.data(), uint32_t(12));
        mcb(HttpResponse::newHttpResponse(drogon::k401Unauthorized, ContentType::CT_NONE));
    }
}
