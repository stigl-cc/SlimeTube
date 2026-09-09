#pragma once
#include <controllers/PageController.h>
#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>

using namespace drogon;

namespace SlimyTube {
    class RecordController : public drogon::HttpController<RecordController> {
        public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(RecordController::registerUser, "/user", Post);
        ADD_METHOD_TO(RecordController::registerUser, "/user", Delete);

        ADD_METHOD_TO(RecordController::registerUser, "/user/name", Post);
        ADD_METHOD_TO(RecordController::registerUser, "/user/password", Post);
    METHOD_LIST_END
        void registerUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void deleteUser(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
