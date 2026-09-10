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
        ADD_METHOD_TO(RecordController::userRegister, "/user", Post);
        ADD_METHOD_TO(RecordController::userDelete, "/user", Delete, "SlimyTube::Authenticator");
        ADD_METHOD_TO(RecordController::userChangeName, "/user/name", Post);
        ADD_METHOD_TO(RecordController::userChangePass, "/user/password", Post);
    METHOD_LIST_END
        void userRegister(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
