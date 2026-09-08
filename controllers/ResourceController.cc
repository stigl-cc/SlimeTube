#include <controllers/ResourceController.h>

#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <filesystem>
#include <system_error>

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    HttpResponsePtr res = HttpResponse::newFileResponse("../videos/video-04.png");
    callback(res);
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    std::string path = "../videos/video-04.mp4";
    std::string header = req->getHeader("Range");
    HttpResponsePtr res;

    std::error_code ec;
    size_t file_length = std::filesystem::file_size(path, ec);

    if(ec) {
        res = HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
        callback(res);
        return;
    }

    std::vector<FileRange> ranges;
    FileRangeParseResult parse_result = FileRangeParseResult::SinglePart;

    if(header.empty()) {
        parse_result = FileRangeParseResult::SinglePart;
        ranges.emplace_back(0, file_length - 1);
    } else parse_result = parseRangeHeader(header, file_length, ranges);

    switch(parse_result) {
        case drogon::InvalidRange:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k400BadRequest);
            callback(res);
            return;

        case drogon::NotSatisfiable:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            callback(res);
            return;

        case drogon::SinglePart:
        case drogon::MultiPart: {
            res = HttpResponse::newFileResponse(path,
                ranges[0].start, ranges[0].end - ranges[0].start, true, "",
                drogon::CT_VIDEO_MP4, "", req);
            callback(res);
            return;
        }
    }
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}
