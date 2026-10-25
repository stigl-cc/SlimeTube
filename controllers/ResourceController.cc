#include <models/video/VideoTranscoder.h>
#include <charconv>
#include <controllers/ResourceController.h>

#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>
#include <endian.h>
#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

#include <format>
#include <fstream>
#include <filesystem>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string_view>
#include <system_error>

#include <models/Attributes.hh>
#include <models/RecordManager.h>
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
    std::shared_lock,
    std::string,
    std::function;

shared_mutex& SlimyTube::ResourceController::getMutexForVideo(const string& key) {
    size_t key_hash = std::hash<std::string>{} (key);
    return videoMutexes[key_hash % videoMutexes.size()];
}

shared_mutex& SlimyTube::ResourceController::getMutexForThumbnail(const string& key) {
    size_t key_hash = std::hash<std::string>{} (key);
    return thumbnailMutexes[key_hash % thumbnailMutexes.size()];
}

std::optional<drogon::HttpStatusCode> SlimyTube::ResourceController::videoIdExists(const std::string& id) const {
    if(id.empty())
        return k400BadRequest;
    if(!SlimyTube::RecordManager::videoExists(id))
        return k404NotFound;
    return std::nullopt;
}

std::optional<drogon::HttpStatusCode> SlimyTube::ResourceController::videoIdWritableAndExists(const std::string& id, const HttpRequestPtr& req) const {
    if(id.empty())
        return k400BadRequest;

    optional<Videos> video = SlimyTube::RecordManager::getVideoIfExists(id);
    if(!video.has_value()) // video not found
        return k404NotFound;

    userId_t userId = req->attributes()->get<userId_t>(SlimyTube::Attributes::userId.data());
    if(video.value().getValueOfChannel() != userId) // video doesn't belong to the user
        return k403Forbidden;
    return std::nullopt;
}

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    std::optional<HttpStatusCode> video_id_check_error = videoIdExists(id);
    if(video_id_check_error.has_value())
        return(void) respond(HttpResponse::newHttpResponse(video_id_check_error.value(), CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id) / "thumbnail";
    shared_lock lk(getMutexForThumbnail(path));
    return(void) respond(HttpResponse::newFileResponse(path, "", CT_IMAGE_JPG, "", req));
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    std::optional<HttpStatusCode> video_id_check_error = videoIdWritableAndExists(id, req);
    if(video_id_check_error.has_value())
        return(void) respond(HttpResponse::newHttpResponse(video_id_check_error.value(), CT_NONE));

    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id) / "thumbnail_blob";
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

    std::optional<Videos> video = RecordManager::getVideoIfExists(id);
    if(!video.has_value() || video.value().getValueOfState() != static_cast<int>(VideoState::Ready))
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));

    std::string quality_suffix;

    const std::vector<char>& resolutions_str = video.value().getValueOfResolutions();
    const int resolution_count = resolutions_str.size() / sizeof(int16_t);
    const int16_t* resolution_shorts = reinterpret_cast<const int16_t*>(resolutions_str.data());

    std::vector<uint16_t> resolutions(resolution_count, 0);
    for(int i =0; i < resolution_count; i++)
        resolutions[i] = le16toh(resolution_shorts[i]);

    const std::string& quality_param = req->getParameter(Query::videoQuality.data());
    if(quality_param.empty())
        quality_suffix = std::to_string(resolutions[0]);
    else {
        int16_t requested_resolution;
        std::from_chars_result offset_param_conv = std::from_chars<int16_t>(quality_param.c_str(), quality_param.c_str() + quality_param.size(), requested_resolution);
        if(offset_param_conv.ec != std::errc()) // quality is not a (valid) number
            return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));
        else if(std::find(resolutions.begin(), resolutions.end(), requested_resolution) == resolutions.end())
            return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k404NotFound, CT_NONE));
        else
            quality_suffix = std::to_string(requested_resolution);
    }

    // TODO: Check quality parameter to determine the suffix of the path
    fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id) / quality_suffix;

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

    // Parse the Range header
    string range_header = req->getHeader(Headers::range.data());
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
            return(void) respond(std::move(res));

        case NotSatisfiable:
            res = HttpResponse::newHttpResponse();
            res->setContentTypeCode(CT_NONE);
            res->setStatusCode(k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            return(void) respond(std::move(res));

        case SinglePart:
        case MultiPart: {
            ranges[0].end = std::min<size_t>(ranges[0].start + Settings::getInstance().limits.getVideoMaxRange, ranges[0].end);
            res = HttpResponse::newFileResponse(path, ranges[0].start, ranges[0].end - ranges[0].start, true, "", CT_VIDEO_MP4, "video/mp4", req);
            if(res->getStatusCode() == HttpStatusCode::k200OK)
                res->setStatusCode(HttpStatusCode::k206PartialContent);
            return(void) respond(std::move(res));
        }
    }
}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, function<void (const HttpResponsePtr &)> &&respond, string id) {
    std::optional<HttpStatusCode> video_id_check_error = videoIdWritableAndExists(id, req);
    if(video_id_check_error.has_value())
        return(void) respond(HttpResponse::newHttpResponse(video_id_check_error.value(), CT_NONE));

    // Parse the X-Offset header
    const std::string& offset_param = req->getHeader(Headers::offset.data());
    size_t offset_header = 0;
    std::from_chars_result offset_param_conv =
        std::from_chars<uint64_t>(offset_param.c_str(), offset_param.c_str() + offset_param.size(), offset_header);
    if(offset_param_conv.ec != std::errc()) // X-Offset is not a (valid) number
        return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k400BadRequest, CT_NONE));

    const fs::path path = fs::weakly_canonical(fs::path(Settings::getInstance().videos_dir)) / fs::path(id) / "blob";
    unique_lock lk(getMutexForVideo(path)); // lock the video file - we are writing to it

    std::error_code ec;
    size_t current_file_size = fs::file_size(path, ec);
    if(ec) {
        if(ec.value() == static_cast<int>(std::errc::no_such_file_or_directory))
            current_file_size = 0;
        else {
            LOG_WARN << ec.message();
            return(void) respond(HttpResponse::newHttpResponse(HttpStatusCode::k500InternalServerError, CT_NONE));
        }
    }

    if(current_file_size < offset_header) { // X-Offset larger than current file size
        HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k424FailedDependency, drogon::CT_NONE);
        res->addHeader(Headers::position.data(), std::to_string(current_file_size));
        return(void) respond(std::move(res));
    }
    const std::string_view& body = req->getBody();
    size_t body_skipped = current_file_size - offset_header;

    bool is_last_param = req->getHeader(Headers::last.data()) == "true";

    if(body_skipped >= body.size() && !is_last_param) { // body already written (X-Offset too small)
        HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k409Conflict, drogon::CT_NONE);
        res->addHeader(Headers::position.data(), std::to_string(current_file_size));
        return(void) respond(std::move(res));
    }

    std::ofstream ofs(path, std::ios::binary | std::ios::out | std::ios::ate | std::ios::app);
    ofs.write(body.data() + body_skipped, body.size() - body_skipped);
    ofs.close();

    lk.unlock();

    HttpResponsePtr res = HttpResponse::newHttpResponse(HttpStatusCode::k202Accepted, drogon::CT_NONE);
    res->addHeader(Headers::position.data(), std::to_string(current_file_size + (body.size() - body_skipped)));
    respond(std::move(res));
    if(!is_last_param)
        return;

    RecordManager::getVideosMapper().updateBy(
        {Videos::Cols::_state},
        Criteria(Videos::Cols::_id, CompareOperator::EQ, id),
        static_cast<int32_t>(VideoState::Encode)
    );
    TranscodeManager::getInstance().enqueueJob(id);
}
