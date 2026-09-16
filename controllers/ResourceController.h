#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/RequestStream.h>

using drogon::HttpMethod::Get,
    drogon::HttpMethod::Post,
    drogon::HttpMethod::Put,
    drogon::HttpRequestPtr,
    drogon::HttpResponsePtr,
    drogon::HttpRequestPtr,
    drogon::RequestStreamPtr,
    drogon::MultipartHeader;

namespace SlimyTube {
    class ResourceController : public drogon::HttpController<ResourceController> {
        private:
        void uploadVideoStreamData(const char*, size_t) const;
        void uploadVideoStreamFinish(std::exception_ptr) const;
        public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ResourceController::getThumbnail, "/rsc/thumbnail/{1}", Get);
        ADD_METHOD_TO(ResourceController::getVideo, "/rsc/video/{1}", Get);

        ADD_METHOD_TO(ResourceController::uploadThumbnail, "/rsc/thumbnail/{1}/upload", Put, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(ResourceController::uploadVideo, "/rsc/video/{1}/upload", Put, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END
        void getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const;
        void getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const;

        void uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const;
        void uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const;
    };
}
