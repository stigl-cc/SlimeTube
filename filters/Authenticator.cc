
#include <filters/Authenticator.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>

#include <models/Session.hh>
#include <models/RecordManager.hh>
#include <models/Attributes.hh>
#include <models/Users.h>
#include <models/Videos.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdexcept>
#include <utility>

using namespace drogon_model::SlimyTube;
namespace orm = drogon::orm;
using
    drogon::HttpStatusCode,
    drogon::ContentType,
    drogon::HttpResponse,
    drogon::HttpResponsePtr;

int SlimyTube::Authenticator::passwordCheck(uint32_t& userId, const std::string& name, const std::string& pass) {
    if(name.empty() || pass.empty()) return 401;

    Mapper<Users> user_mapper = RecordManager::getUserMapper();
    std::vector<Users> users = user_mapper.findBy(orm::Criteria(Users::Cols::_name, drogon::orm::CompareOperator::EQ, name));
    if(users.empty()) return 401;

    std::vector<char>
        salt = users[0].getValueOfSalt(),
        correct_hash = users[0].getValueOfHash();

    uint8_t hash[32];
    try {
        RecordManager::createHash(pass, std::span<const uint8_t, 16>(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()), hash);
    } catch(const std::runtime_error& e) {
        LOG_WARN << "Authenticator failed with " << e.what();
        return 500;
    }

    if(CRYPTO_memcmp(hash, correct_hash.data(), correct_hash.size())) return 401;

    userId = users[0].getValueOfId();
    return 0;
}

void SlimyTube::Authenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb, bool strict) {
    uint32_t userId;
    Attributes::LoginMethod loginMethod;

    // user already logged in through session
    if(req->session()->find(Session::keyUserId.data())) {
        userId = req->session()->get<uint32_t>(Session::keyUserId.data());
        loginMethod = Attributes::LoginMethod::Session;
    }
    // log in using name + password
    else {
        const std::string&
            name = req->getParameter("name"),
            pass = req->getParameter("pass");

        int http_code = passwordCheck(userId, name, pass);
        if(http_code) { // authentication failed
            if(strict) {
                mcb(HttpResponse::newHttpResponse(static_cast<HttpStatusCode>(http_code), ContentType::CT_NONE));
                return;
            } else
                loginMethod = Attributes::LoginMethod::LaxNone;
        } else
            loginMethod = Attributes::LoginMethod::Password;
    }

    // pass through attributes
    req->attributes()->insert(Attributes::keyLoginMethod.data(), loginMethod);
    if(loginMethod != Attributes::LoginMethod::LaxNone)
        req->attributes()->insert(Attributes::keyUserId.data(), userId);

    nextCb([mcb = std::move(mcb)](const HttpResponsePtr &resp) { mcb(resp); });
}

void SlimyTube::StrictAuthenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    return Authenticator::invoke(std::move(req), std::move(nextCb), std::move(mcb), true);
}

void SlimyTube::LaxAuthenticator::invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) {
    return Authenticator::invoke(std::move(req), std::move(nextCb), std::move(mcb), false);
}
