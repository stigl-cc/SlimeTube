#include "Attributes.hh"
#include <controllers/ResourceController.h>

#include <drogon/MultiPart.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>
#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

#include <format>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <system_error>

#include <models/RecordManager.hh>
#include <models/Settings.hh>

using namespace SlimyTube::Attributes;

using namespace drogon;
using namespace drogon::orm;
namespace fs = std::filesystem;

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {
    if(!RecordManager::videoExists(id)) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
        return;
    }
    respond(HttpResponse::newFileResponse(fs::path(Settings::getInstance().thumbnails_dir) / fs::path(id)));
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {
    if(!RecordManager::videoExists(id)) {
        respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
        return;
    }

    fs::path path = fs::path(Settings::getInstance().videos_dir) / fs::path(id);
    HttpResponsePtr res;

    std::error_code ec;
    size_t file_length = std::filesystem::file_size(path, ec);

    if(ec) {
        if(ec.value() == static_cast<int>(std::errc::no_such_file_or_directory))
            respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
        else {
            LOG_WARN << "Error when requesting video" << ec.message();
            respond(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, CT_NONE));
        }
        return;
    }

    std::string range_header = req->getHeader("Range");
    if(range_header.empty()) {
        res = HttpResponse::newFileResponse(path, "", CT_VIDEO_MP4, "video/mp4", req);
        res->addHeader("Accept-Ranges", "bytes");
        respond(res);
        return;
    }

    std::vector<FileRange> ranges;
    FileRangeParseResult parse_result = parseRangeHeader(range_header, file_length, ranges);

    switch(parse_result) {
        case InvalidRange:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(CT_NONE);
            res->setStatusCode(k400BadRequest);
            respond(res);
            return;

        case NotSatisfiable:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(CT_NONE);
            res->setStatusCode(k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            respond(res);
            return;

        case SinglePart:
        case MultiPart: {
            ranges[0].end = std::min<size_t>(ranges[0].start + Settings::getInstance().zeroRangeLimit, ranges[0].end);
            res = HttpResponse::newFileResponse(path, ranges[0].start, ranges[0].end - ranges[0].start, true, "", CT_VIDEO_MP4, "video/mp4", req);
            if(res->getStatusCode() == HttpStatusCode::k200OK)
                res->setStatusCode(HttpStatusCode::k206PartialContent);
            respond(res);
            return;
        }
    }
}


void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {
    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();
    Videos video;
    try {
        video = video_mapper.findOne(Criteria(Videos::Cols::_id, CompareOperator::EQ, id));
    } catch(UnexpectedRows e) { // video id doesn't exist
        respond(HttpResponse::newHttpResponse(drogon::k404NotFound, CT_NONE));
        return;
    }

    if(video.getValueOfChannel() != req->attributes()->get<userId_t>(keyUserId.data())) { // video doesn't belong to the user
        respond(HttpResponse::newHttpResponse(drogon::k403Forbidden, CT_NONE));
        return;
    }

    fs::path path = fs::path(Settings::getInstance().thumbnails_dir) / fs::path(id);
    std::ofstream ofs(path, std::ios::out | std::ios::binary);

    const std::string_view& body = req->getBody();
    ofs.write(body.data(), body.size());
    ofs.close();

    respond(HttpResponse::newHttpResponse(drogon::k202Accepted, CT_NONE));
}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, drogon::RequestStreamPtr &&streamCtx, std::function<void (const HttpResponsePtr &)> &&respond, std::string id) const {

}
