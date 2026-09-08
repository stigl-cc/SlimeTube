#include "ResourceController.h"

#include <lib/drogon/lib/src/RangeParser.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <fstream>

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    HttpResponsePtr res = HttpResponse::newFileResponse("../videos/video-01.png");
    callback(res);
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    std::string header = req.get()->getHeader("Range");

    HttpResponsePtr res = HttpResponse::newHttpResponse();

    std::ifstream ifs("../videos/video-01.mp4", std::ios::in | std::ios::binary | std::ios::ate);
    if(!ifs.is_open()) {
        res->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
        callback(res);
        return;
    }

    ssize_t file_length = ifs.tellg();

    std::vector<FileRange> ranges;

    switch(parseRangeHeader(header, file_length, ranges)) {
        case drogon::InvalidRange:
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k400BadRequest);
            callback(res);
            return;

        case drogon::NotSatisfiable:
            res->setContentTypeCode(drogon::CT_NONE);
            res->setStatusCode(drogon::k416RequestedRangeNotSatisfiable);
            res->addHeader("Content-Range", std::format("bytes */{}", file_length));
            callback(res);
            return;

        case drogon::SinglePart: {
            const FileRange& range = ranges[0];
            std::string video_buffer(range.end - range.start, 0);

            ifs.seekg(range.start, std::ios::beg);
            ifs.read(video_buffer.data(), video_buffer.size());

            res->setStatusCode(drogon::k206PartialContent);
            res->setContentTypeCode(drogon::CT_VIDEO_MP4);
            res->addHeader("Content-Range", std::format("bytes {}-{}/{}", ranges[0].start, ranges[0].end, file_length));
            res->addHeader("Accept-Ranges", "bytes");
            res->setBody(video_buffer);
            callback(res);
            return;
        }
        case drogon::MultiPart: {
            size_t total_lenght = 0;
            for(const FileRange& range : ranges)
                total_lenght += range.end - range.start;

            std::ostringstream oss;
            for(const FileRange& range : ranges) {
                std::string str(range.end - range.start, '0');

                ifs.seekg(range.start, std::ios::beg);
                ifs.read(str.data(), str.size());
                oss
                    << "--DR_BOUNDARY_3214\r\n"
                    << "";
                oss.write(str.data(), str.size());
            }

            if(ifs.fail()) {
                ifs.close();

                res->setStatusCode(drogon::HttpStatusCode::k500InternalServerError);
                callback(res);
                return;
            }

            ifs.close();

            res->addHeader("Accept-Ranges", "bytes");
            res->setStatusCode(drogon::k206PartialContent);
            res->setContentTypeCode(drogon::CT_VIDEO_MP4);
            res->setBody(oss.str());
            callback(res);
            break;
        }
    }
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}
