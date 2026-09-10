#include <controllers/ResourceController.h>

#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <filesystem>
#include <system_error>

using namespace drogon;

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {
    HttpResponsePtr res = drogon::HttpResponse::newFileResponse("../videos/video-04.png");
    respond(res);
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {
    std::string path = "../videos/video-04.mp4";
    std::string header = req->getHeader("Range");
    HttpResponsePtr res;

    std::error_code ec;
    size_t file_length = std::filesystem::file_size(path, ec);

    if(ec) {
        res = HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
        respond(res);
        return;
    }

    if(header.empty()) {
        res = HttpResponse::newFileResponse(path, "", drogon::CT_VIDEO_MP4, "video/mp4", req);
        respond(res);
        return;
    }

    std::vector<FileRange> ranges;
    FileRangeParseResult parse_result = parseRangeHeader(header, file_length, ranges);

    switch(parse_result) {
        case drogon::InvalidRange:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k400BadRequest);
            respond(res);
            return;

        case drogon::NotSatisfiable:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            respond(res);
            return;

        case drogon::SinglePart:
        case drogon::MultiPart: {
            // TODO: Impose a size limit for the 0- requests
            //ranges[0].end = std::min(ranges[0].start + 1024 * 1024, ranges[0].end);
            res = HttpResponse::newFileResponse(path, ranges[0].start, ranges[0].end - ranges[0].start, true, "", drogon::CT_VIDEO_MP4, "video/mp4", req);
            if(res->getStatusCode() == HttpStatusCode::k200OK)
                res->setStatusCode(HttpStatusCode::k206PartialContent);
            respond(res);
            return;
        }
    }
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {

}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {

}
