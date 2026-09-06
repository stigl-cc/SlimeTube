#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace SlimyTube {
    class ResourceController : public drogon::HttpController<ResourceController> {
        public:
    METHOD_LIST_BEGIN
        METHOD_ADD(ResourceController::getThumbnail, "/thumbnail", Get);
        METHOD_ADD(ResourceController::getVideo, "/video", Get);

        METHOD_ADD(ResourceController::uploadThumbnail, "/thumbnail", Put);
        METHOD_ADD(ResourceController::uploadVideo, "/video", Put);
    METHOD_LIST_END
        void getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;

        void uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
