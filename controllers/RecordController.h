#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

using drogon::HttpMethod::Get,
    drogon::HttpMethod::Post,
    drogon::HttpMethod::Delete,
    drogon::HttpRequestPtr,
    drogon::HttpResponsePtr;

namespace SlimyTube {
    class RecordController : public drogon::HttpController<RecordController> {
        public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(RecordController::userRegister, "/register", Post);
        ADD_METHOD_TO(RecordController::userLogin, "/login", Post, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(RecordController::userLogout, "/logout", Post, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(RecordController::userDelete, "/user", Delete, "SlimyTube::StrictAuthenticator");

        ADD_METHOD_TO(RecordController::userChangeName, "/user/name", Post, "SlimyTube::StrictAuthenticator");
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
