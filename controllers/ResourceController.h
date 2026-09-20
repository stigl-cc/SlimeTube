#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/RequestStream.h>
#include <shared_mutex>

namespace SlimyTube {
    using drogon::HttpMethod::Get,
        drogon::HttpMethod::Post,
        drogon::HttpMethod::Put,
        drogon::HttpRequestPtr,
        drogon::HttpResponsePtr;

    class ResourceController : public drogon::HttpController<ResourceController> {
        static constexpr size_t mutexCount = 128;
        // TODO: Handle mutex overflow
        std::array<std::shared_mutex, mutexCount> videoMutexes;
        std::array<std::shared_mutex, mutexCount> thumbnailMutexes;

        std::shared_mutex& getMutexForThumbnail(const std::string& key);
        std::shared_mutex& getMutexForVideo(const std::string& key);
        public:
    METHOD_LIST_BEGIN
        // errors: 404
        // success: 200
        ADD_METHOD_TO(ResourceController::getThumbnail, "/rsc/thumbnail/{1}", Get);

        // errors: 404, 416, 500
        // success: 200 206
        ADD_METHOD_TO(ResourceController::getVideo, "/rsc/video/{1}", Get);

        // errors:  404 403(video.channel != user)
        // success: 202
        ADD_METHOD_TO(ResourceController::uploadThumbnail, "/rsc/thumbnail/{1}/upload", Put, "SlimyTube::StrictAuthenticator");

        // headers: X-Offset
        // errors: 404 403(video.channel != user) 412(X-Offset > fileLen)
        // success: 202
        ADD_METHOD_TO(ResourceController::uploadVideo, "/rsc/video/{1}", Put, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END
        void getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id);
        void getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id);

        void uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id);
        void uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id);
    };
}
