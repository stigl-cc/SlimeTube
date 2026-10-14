#include <drogon/orm/Criteria.h>
#include <controllers/PageController.h>
#include <controllers/ResourceController.h>

#include <json/value.h>
#include <limits>
#include <models/Settings.hh>
#include <models/Videos.h>
#include <models/TimeUtility.h>
#include <models/VideoTranscoder.h>
#include <models/RecordManager.h>
#include <models/VideoTypes.h>

#include <drogon/Cookie.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <drogon/RateLimiter.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;
using namespace SlimyTube;
using namespace drogon::orm;
using namespace drogon_model::SlimyTube;
using
    drogon::HttpViewData,
    drogon::HttpResponse,
    drogon::Cookie,
    drogon::app;

// Delete all videos that have not finished uploading after a day from creation
void cleanupVideos() {
    const uint64_t min_time = TimeUtil::GetUnixSecond() - TimeConstants::day;

    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();
    std::vector<Videos> videos_to_delete = video_mapper.findBy(
        Criteria(Videos::Cols::_state, CompareOperator::EQ, static_cast<int32_t>(VideoState::Upload)) &&
        Criteria(Videos::Cols::_creation_time, CompareOperator::LT, min_time)
    );

    for(const Videos& video_to_delete : videos_to_delete)
        RecordManager::deleteVideo(video_to_delete);
}


const std::vector<std::string> downloadable_file_types {
    "html",
    "js",
    "css",
    "ico",
};

const std::vector<std::pair<std::string, std::string>> ssl_commands {
    { "Options", "Compression" }
};

const std::vector<std::pair<std::string, std::string>> static_file_headers {
    { "Access-Control-Allow-Origin", "*" },
};

int main() {
    Settings& settings = Settings::getInstance();
    app().loadConfigFile("data/config.json");

    if(settings.parse(app().getCustomConfig()) != 0)
        return 1;

    HttpViewData data = Settings::getInstance().getData();
    data.insert("error-text", "404 Not Found");
    data.insert("back", "/");

    const drogon::HttpResponsePtr
        res404 = HttpResponse::newHttpViewResponse("error", std::move(data));

    app().setCustom404Page(std::move(res404), true);

    app()
        .setSSLConfigCommands(ssl_commands)
        .setFileTypes(downloadable_file_types)
        .setStaticFileHeaders(static_file_headers);

    app()
        .setFloatPrecisionInJson(0, "significant")
        .setUnicodeEscapingInJson(true)
        .setJsonParserStackLimit(1000);

    app()
        .setServerHeaderField(settings.title)
        .enableServerHeader(true)
        .enableDateHeader(true)
        .enableReusePort(true);

    app()
        .setUploadPath("../data/uploads")
        .setDocumentRoot("./static/")
        .setImplicitPageEnable(false)
        .enableSession(604'800, Cookie::SameSite::kLax, "SessionId", 2'628'000); // timeout = week, max_age = month

    app()
        .setGzipStatic(true)
        .setBrStatic(true)
        .enableGzip(true)
        .enableBrotli(false)
        .setStaticFilesCacheTime(60)
        .enableSendfile(true);

    app()
        .setClientMaxBodySize(10'485'760)
        .setClientMaxMemoryBodySize(10'485'760)
        .setMaxConnectionNum(10'000)
        .setMaxConnectionNumPerIP(10);

    app().getLoop()
        ->runEvery(3'600, cleanupVideos);

    // Start TranscodeManager thread
    TranscodeManager::getInstance();

    app().run();
    return 0;
}
