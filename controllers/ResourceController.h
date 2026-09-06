#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace SlimyTube {
    class ResourceController : public drogon::HttpController<ResourceController> {
        public:
    METHOD_LIST_BEGIN
        METHOD_ADD(ResourceController::getThumbnail, "/thumbnail/{1}", Get);
        METHOD_ADD(ResourceController::getVideo, "/video/{1}", Get);

        METHOD_ADD(ResourceController::uploadThumbnail, "/thumbnail/{1}", Put);
        METHOD_ADD(ResourceController::uploadVideo, "/video/{1}", Put);
    METHOD_LIST_END
        void getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;

        void uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
        void uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    };
}
