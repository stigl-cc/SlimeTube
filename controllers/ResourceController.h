#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/RequestStream.h>
#include <shared_mutex>

#include <models/ViewCountManager.h>
#include <models/video/VideoTranscoder.h>
#include <models/Settings.h>
#include <models/Videos.h>

namespace SlimyTube {
    using drogon::HttpMethod::Get,
        drogon::HttpMethod::Post,
        drogon::HttpMethod::Put,
        drogon::HttpRequestPtr,
        drogon::HttpResponsePtr;

    class ResourceController : public drogon::HttpController<ResourceController> {
        const Settings& mSettings;
        ViewCountManager& mViewCountManager;

        static constexpr size_t mutexCount = 128;

        std::array<std::shared_mutex, mutexCount> videoMutexes;
        std::array<std::shared_mutex, mutexCount> thumbnailMutexes;

        std::shared_mutex& getMutexForThumbnail(const std::string& key);
        std::shared_mutex& getMutexForVideo(const std::string& key);

        std::optional<drogon::HttpStatusCode> videoIdExists(const std::string& id) const;
        std::optional<drogon::HttpStatusCode> videoIdWritableAndExists(const std::string& id, const HttpRequestPtr& req, drogon_model::SlimyTube::Videos& video_ref) const;
        public:
    METHOD_LIST_BEGIN
        // errors: 404
        // success: 200
        ADD_METHOD_TO(ResourceController::getThumbnail, "/rsc/thumbnail/{1}", Get);

        // errors: 404, 416, 500
        // success: 200 206
        ADD_METHOD_TO(ResourceController::getVideo, "/rsc/video/{1}", Get);

        // errors: 401 404 403(video.channel != user)
        // success: 202
        ADD_METHOD_TO(ResourceController::uploadThumbnail, "/rsc/thumbnail/{1}", Put, "SlimyTube::StrictAuthenticator");

        ADD_METHOD_TO(ResourceController::editThumbnail, "/rsc/thumbnail/{1}", Post, "SlimyTube::StrictAuthenticator");

        // headers: X-Offset
        // r-headers: X-Position
        // errors: 401 404 403(video.channel != user) 409(X-Offset too small) 424(X-Offset too big)
        // success: 202
        ADD_METHOD_TO(ResourceController::uploadVideo, "/rsc/video/{1}", Put, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END
        void getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& id);
        void getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& id);

        void editThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& id);
        void uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& id);
        void uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& id);
        ResourceController();
    };
}
