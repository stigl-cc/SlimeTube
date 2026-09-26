#include <RecordManager.hh>
#include <chrono>
#include <drogon/orm/Criteria.h>
#include <models/Settings.hh>
#include <controllers/PageController.h>
#include <controllers/ResourceController.h>
#include <models/Videos.h>

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
    drogon::app,
    SlimyTube::Settings;

bool ensureDirectory(fs::path p) {
    std::error_code ec;
    std::filesystem::file_status status = fs::status(p, ec);

    if (!fs::exists(p)) {
        fs::create_directory(p, ec);
        if(ec != std::errc())
            LOG_ERROR << "Failed to create directory " << fs::canonical(p) << ": " << ec.message();
    } else if (!fs::is_directory(status)) {
        LOG_ERROR << fs::canonical(p) << ": path exists but is not a directory.";
        return false;
    }
    return true;
}

using namespace std::chrono;
void cleanupVideos() {
    const uint64_t
        time = time_point_cast<std::chrono::seconds>(system_clock::now()).time_since_epoch().count() - 86'400;

    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();
    std::vector<Videos> videos_to_delete = video_mapper.findBy(
        Criteria(Videos::Cols::_state, CompareOperator::EQ, static_cast<int32_t>(VideoState::Upload)) &&
        Criteria(Videos::Cols::_creation_time, CompareOperator::LT, time)
    );

    for(const Videos& video_to_delete : videos_to_delete)
        RecordManager::deleteVideo(video_to_delete);
}

int main() {
    Settings& settings = Settings::getInstance();
    app().loadConfigFile("data/config.json");

    Json::Value custom_user_config = app().getCustomConfig();
    settings.title = custom_user_config.get("title", "SlimyTube").as<std::string>();

    settings.zeroRangeLimit = custom_user_config.get("zero_range_limit", 10 * 1024 * 1024).as<uint64_t>();
    settings.salt_len = custom_user_config.get("salt_len", 16).as<size_t>();

    settings.thumbnails_dir = custom_user_config.get("thumbnails_dir", "data/thumbnails").as<std::string>();
    if(!ensureDirectory(settings.thumbnails_dir))
        return 1;

    settings.videos_dir = custom_user_config.get("videos_dir", "data/videos").as<std::string>();
    if(!ensureDirectory(settings.videos_dir))
        return 1;

    settings.blobs_dir = custom_user_config.get("blobs_dir", "data/blobs").as<std::string>();
    if(!ensureDirectory(settings.blobs_dir))
        return 1;

    HttpViewData data = Settings::getInstance().getData();
    data.insert("error-text", "404 Not Found");
    data.insert("back", "/");

    const drogon::HttpResponsePtr
        res404 = HttpResponse::newHttpViewResponse("error", std::move(data));

    app().setSSLConfigCommands({
            { "Options", "Compression" }
        });

    app().getLoop()->runEvery(3'600, cleanupVideos );

    app().setFileTypes({
            "html",
            "js",
            "css",
            "ico",
        });

    app().setStaticFileHeaders({
            { "Access-Control-Allow-Origin", "*" },
        });


    app().setCustom404Page(std::move(res404), true);

    app()
        .setServerHeaderField(settings.title);

    app()
        .setFloatPrecisionInJson(0, "significant")
        .setUnicodeEscapingInJson(true)
        .setJsonParserStackLimit(1000);

    app()
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
        .setClientMaxWebSocketMessageSize(131'072);

    app()
        .setMaxConnectionNum(10'000)
        .setMaxConnectionNumPerIP(10);

    app().run();
    return 0;
}
