#include <controllers/ResourceController.h>

#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>
#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

#include <format>
#include <fstream>
#include <filesystem>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string_view>
#include <system_error>

#include <models/Attributes.hh>
#include <models/RecordManager.hh>
#include <models/Settings.hh>

using namespace SlimyTube::Attributes;
using namespace std::placeholders;

using namespace drogon;
using namespace drogon::orm;
namespace fs = std::filesystem;
using

    std::vector,
    std::optional,
    std::shared_mutex,
    std::unique_lock,
    std::shared_lock;

shared_mutex& SlimyTube::ResourceController::getMutexForVideo(const string& key) {
    shared_lock lk(mutexVideoMutexes);
    unordered_map<string, unique_ptr<shared_mutex>>::iterator it = mutexVideos.find(key);
    if(it == mutexVideos.end()) {
        lk.unlock(); // prevent deadlock
        unique_lock u_lk(mutexVideoMutexes);
        it = mutexVideos.emplace().first;

        lk.lock();
    }
    return *it->second.get();
}

shared_mutex& SlimyTube::ResourceController::getMutexForThumbnail(const string& key) {
    shared_lock lk(mutexThumbnailMutexes);
    unordered_map<string, unique_ptr<shared_mutex>>::iterator it = mutexThumbnails.find(key);
    if(it == mutexThumbnails.end()) {
        lk.unlock(); // prevent deadlock
        unique_lock u_lk(mutexThumbnailMutexes);
        it = mutexThumbnails.emplace().first;

        lk.lock();
    }
    return *it->second.get();
}

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    if(id.empty())
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));
    if(!RecordManager::videoExists(id))
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().thumbnails_dir)) / fs::path(id);
    shared_lock lk(getMutexForThumbnail(path));
    return(void) respond(HttpResponse::newFileResponse(path, "", CT_IMAGE_JPG, "", req));
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    if(id.empty())
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));

    optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value()) // video not found
        return(void) respond(HttpResponse::newHttpResponse(drogon::k404NotFound, CT_NONE));
    if(video.value().getValueOfChannel() != req->attributes()->get<userId_t>(keyUserId.data())) // video doesn't belong to the user
        return(void) respond(HttpResponse::newHttpResponse(drogon::k403Forbidden, CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().thumbnails_dir)) / fs::path(id);
    unique_lock lk(getMutexForThumbnail(path)); // lock the thumbnail file - we are writing to it

    // TODO: Make sure its an image and convert all to jpeg

    std::ofstream ofs(path, std::ios::out | std::ios::binary);
    const std::string_view& body = req->getBody();
    ofs.write(body.data(), body.size());
    ofs.close();

    respond(HttpResponse::newHttpResponse(HttpStatusCode::k202Accepted, CT_NONE));
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    if(id.empty())
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));
    if(!RecordManager::videoExists(id))
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id);

    shared_lock lk(getMutexForVideo(path));
    HttpResponsePtr res;

    std::error_code ec;
    size_t file_length = fs::file_size(path, ec);

    if(ec) {
        if(ec.value() == static_cast<int>(std::errc::no_such_file_or_directory))
            return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
        LOG_WARN << "Error when requesting video" << ec.message();
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, CT_NONE));
    }

    string range_header = req->getHeader("Range");
    if(range_header.empty()) {
        res = HttpResponse::newFileResponse(path, "", CT_VIDEO_MP4, "video/mp4", req);
        res->addHeader("Accept-Ranges", "bytes");
        return(void) respond(res);
    }

    vector<FileRange> ranges;
    FileRangeParseResult parse_result = parseRangeHeader(range_header, file_length, ranges);

    switch(parse_result) {
        case InvalidRange:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(CT_NONE);
            res->setStatusCode(k400BadRequest);
            return(void) respond(res);

        case NotSatisfiable:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(CT_NONE);
            res->setStatusCode(k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            return(void) respond(res);

        case SinglePart:
        case MultiPart: {
            ranges[0].end = std::min<size_t>(ranges[0].start + Settings::getInstance().zeroRangeLimit, ranges[0].end);
            res = HttpResponse::newFileResponse(path, ranges[0].start, ranges[0].end - ranges[0].start, true, "", CT_VIDEO_MP4, "video/mp4", req);
            if(res->getStatusCode() == HttpStatusCode::k200OK)
                res->setStatusCode(HttpStatusCode::k206PartialContent);
            return(void) respond(res);
        }
    }
}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    if(id.empty())
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));

    optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value()) // video not found
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
    if(video.value().getValueOfChannel() != req->attributes()->get<userId_t>(keyUserId.data())) // video doesn't belong to the user
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k403Forbidden, CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id);
    unique_lock lk(getMutexForVideo(path)); // lock the video file - we are writing to it

    const std::string& offsetParam = req->getParameter("X-Offset");

    return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k200OK, CT_NONE));
}
