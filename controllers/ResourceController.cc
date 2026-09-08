#include "ResourceController.h"

#include <algorithm>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <sstream>

using std::optional;

inline optional<int64_t> parse_int(const std::string &str) {
    std::istringstream iss(str);
    int64_t value;

    iss >> value;
    if (iss.fail() || !iss.eof()) {
        return {};
    }

    return value;
}
inline void trim(std::string &str) {
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ') + 1);
}

uint32_t parseRangeHeader(const HttpRequestPtr& req, std::vector<std::pair<size_t, size_t>>& ranges, ssize_t body_size) {
    std::string header = req.get()->getHeader("Range");

    size_t unitIndex = header.find('=');
    if (unitIndex == std::string::npos)
        return 400;

    std::string unit = header.substr(0, unitIndex);
    std::stringstream ss(header.substr(unitIndex + 1));
    std::string segment;

    while(std::getline(ss, segment, ',')) {
        size_t index = segment.find('-');
        if(index == std::string::npos)
            return 400;

        std::string
            start = segment.substr(0, index),
            end = segment.substr(index + 1);

        trim(start);
        trim(end);

        optional<int64_t>
            start_int = parse_int(start),
            end_int = parse_int(end);

        if (start.empty()) {
            if (end.empty() || end.at(0) == '-') // handles "" or "-"
                return 400;

            if(!end_int.has_value())
                return 400;
            ssize_t endOffset = std::min<int64_t>(body_size, end_int.value());

            // "-20"
            ranges.emplace_back(
                body_size - endOffset,
                endOffset);
        } else {
            if(!start_int.has_value())
                return 400;
            if(start_int.value() >= body_size)
                return 416;

            if (end.empty()) {
                // "10-"
                ranges.emplace_back(
                    start_int.value(),
                    body_size - start_int.value()
                );
            } else {
                if(!end_int.has_value() || start_int < 0)
                    return 400;
                if(start_int.value() > end_int.value())
                    return 416;

                // "10-20"
                ranges.emplace_back(
                    start_int.value(),
                    std::min<ssize_t>(body_size, end_int.value() + 1) - start_int.value()
                );
            }
        }
    }
    return 200;
}

void SlimyTube::ResourceController::getThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    HttpResponsePtr res = HttpResponse::newFileResponse("../videos/video-01.png");
    callback(res);
}

void SlimyTube::ResourceController::getVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {
    std::vector<std::pair<size_t, size_t>> ranges;
    int code = parseRangeHeader(req, ranges, 9000);

    for(auto range : ranges) {
        std::cout << range.first << " " << range.second << "\n";
    }
    HttpResponsePtr res = HttpResponse::newFileResponse("../videos/video-01.mp4", -100, 10, true, "", drogon::CT_NONE, "video/mp4", req);
    callback(res);
}

void SlimyTube::ResourceController::uploadThumbnail(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}

void SlimyTube::ResourceController::uploadVideo(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, std::string id) const {

}
