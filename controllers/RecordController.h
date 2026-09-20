#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

namespace SlimyTube {
    using drogon::HttpMethod::Get,
        drogon::HttpMethod::Post,
        drogon::HttpMethod::Delete,
        drogon::HttpRequestPtr,
        drogon::HttpResponsePtr;

    class RecordController : public drogon::HttpController<RecordController> {
        public:
    METHOD_LIST_BEGIN
        // headers: name, pass
        // errors: 400(name/pass headers missing) 409(name exists) 500(ssl failed)
        // success: 201
        ADD_METHOD_TO(RecordController::userRegister, "/register", Post, "SlimyTube::RateLimiter");

        // errors: 429 303(wrong password)
        // success: 303(reset /account)
        ADD_METHOD_TO(RecordController::userLogin, "/login", Post, "SlimyTube::LaxAuthenticator", "SlimyTube::RateLimiter");

        // headers: Cookie
        // errors: 401 400(logged in via non session)
        // success: 303(go to login.csp)
        ADD_METHOD_TO(RecordController::userLogout, "/logout", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401
        // success: 200
        ADD_METHOD_TO(RecordController::userDelete, "/user", Delete, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401 400(new_name empty/missing) 409(name exists)
        // success: 200
        ADD_METHOD_TO(RecordController::userChangeName, "/user/name", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401 400(new_pass empty/missing) 500(ssl failed)
        // success: 303(reset /account)
        ADD_METHOD_TO(RecordController::userChangePass, "/user/password", Post, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END

        void userRegister(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userLogin(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userLogout(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
    };
}
