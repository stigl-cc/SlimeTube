#include <filters/Authenticator.h>
#include <Session.hh>

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>

#include <models/Users.h>
#include <models/Videos.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/err.h>


using namespace drogon_model::SlimyTube;
namespace orm = drogon::orm;
using
    drogon::HttpStatusCode,
    drogon::ContentType,
    drogon::HttpResponse,
    drogon::HttpResponsePtr;

void SlimyTube::Authenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    orm::DbClientPtr db = drogon::app().getDbClient();
    orm::Mapper<Users> user_mapper(db);

    uint32_t userId;
    // user already logged in through session
    if(req->session()->find(Session::keyUserId.data())) {
        userId = req->session()->get<uint32_t>(Session::keyUserId.data());
    }
    // log in using name + password
    else {
        std::string
            name = req->getParameter("name"),
            pass = req->getParameter("pass");

        if(name.empty() || pass.empty()) {
            mcb(HttpResponse::newHttpResponse(HttpStatusCode::k401Unauthorized, ContentType::CT_NONE));
            return;
        }

        std::vector<Users> users = user_mapper.findBy(orm::Criteria(Users::Cols::_name, drogon::orm::CompareOperator::EQ, name));
        if(users.empty()) {
            mcb(HttpResponse::newHttpResponse(HttpStatusCode::k401Unauthorized, ContentType::CT_NONE));
            return;
        }

        std::vector<char>
            salt = users[0].getValueOfSalt(),
            correct_hash = users[0].getValueOfHash();

        uint8_t hash[32];

        if(!PKCS5_PBKDF2_HMAC(
                pass.data(), pass.size(),
                reinterpret_cast<const uint8_t*>(salt.data()), salt.size(),
                1024, EVP_sha256(),
                sizeof(hash), hash)) {
            LOG_WARN << "SHA256 Failed";
            ERR_print_errors_fp(stderr);

            mcb(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, ContentType::CT_NONE));
            return;
        }

        if(CRYPTO_memcmp(hash, correct_hash.data(), correct_hash.size())) {
            mcb(HttpResponse::newHttpResponse(HttpStatusCode::k401Unauthorized, ContentType::CT_NONE));
            return;
        }

        // authentication successful
        userId = users[0].getValueOfId();
        req->session()->insert(Session::keyUserId.data(), uint32_t(userId));
    }

    req->attributes()->insert("userId", userId);
    nextCb([mcb = std::move(mcb)](const HttpResponsePtr &resp) { mcb(resp); });
}
