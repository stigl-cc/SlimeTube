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
        ADD_METHOD_TO(RecordController::registerUser, "/user", Post);
        ADD_METHOD_TO(RecordController::deleteUser, "/user", Delete);

        ADD_METHOD_TO(RecordController::registerUser, "/user/name", Post);
        ADD_METHOD_TO(RecordController::registerUser, "/user/password", Post);
    METHOD_LIST_END
        void registerUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void deleteUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
