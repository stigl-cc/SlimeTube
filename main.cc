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

void parseLimits(Json::Value config, Limits& limits) {
    Json::Value user_limit_config = config.get("limits", {});

    // max of int32 - 2 GB
    constexpr ssize_t max_per_default_capacity = std::numeric_limits<int32_t>::max();

    limits.blobSize = user_limit_config.get("max_blob_size", max_per_default_capacity).as<ssize_t>();
    if(limits.blobSize == -1)
        limits.blobSize = std::numeric_limits<ssize_t>::max();

    limits.totalUserSize = user_limit_config.get("max_user_size", max_per_default_capacity).as<ssize_t>();
    if(limits.totalUserSize == -1)
        limits.totalUserSize = std::numeric_limits<ssize_t>::max();

    limits.getVideoMaxRange = user_limit_config.get("max_get_video_range_size", 10 * 1024 * 1024).as<ssize_t>();
    if(limits.getVideoMaxRange == -1)
        limits.getVideoMaxRange = std::numeric_limits<ssize_t>::max();

    limits.aspectRatio = user_limit_config.get("max_aspect_ratio", 10).as<float>();
    if(std::fabs(limits.aspectRatio + 1) <= std::numeric_limits<float>::epsilon())
        limits.aspectRatio = std::numeric_limits<float>::max();

    limits.max_input.w = user_limit_config.get("max_input_video_width", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
    limits.max_input.h = user_limit_config.get("max_input_video_height", std::numeric_limits<dimension_t>::max()).as<dimension_t>();

    limits.max_encode.w = user_limit_config.get("max_encode_video_width", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
    limits.max_encode.h = user_limit_config.get("max_encode_video_height", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
}

int parseSettings(Settings& settings) {
    Json::Value custom_user_config = app().getCustomConfig();

    settings.title = custom_user_config.get("title", "SlimyTube").as<std::string>();
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

    settings.allow_unlisted_ratios = custom_user_config.get("allow_unlisted_ratios", false).as<bool>();
    settings.default_unlisted_framerate = custom_user_config.get("default_unlisted_framerate", 30).as<int>();

    for(const Json::Value& resolution : custom_user_config.get("resolutions", {})) {
        SupportedTarget target;
        try {
            if(resolution.size() < 6)
                throw Json::LogicError("config resolution format not recognized, please use [[ratio_n, ratio_d], w, h, r, br, also_encode],");
            target.ratio_num = resolution[0][0].asInt();
            target.ratio_den = resolution[0][1].asInt();

            settings.supported_ratios.emplace(target.ratio_num, target.ratio_den);

            target.width = resolution[1].asInt();
            target.height = resolution[2].asInt();

            target.max_framerate = resolution[3].asInt();
            target.max_bitrate = resolution[4].asInt();
            for(const Json::Value& also : resolution[5])
                target.also_encode.push_back(also.asInt());
        } catch(const Json::LogicError& ex) {
            LOG_ERROR << "Error while processing resolution " << resolution.toStyledString() << "" << ex.what();
            return 1;
        }
    }

    // convert also_encode heights to their indexes (O(n^2) - in practice O(n))
    for(int n =0; n < settings.supported_targets.size(); n++) {
        SupportedTarget& target = settings.supported_targets[n];

        int also_encode_i = 0;
        if(target.also_encode.empty())
            continue;

        for(int i =n; i < settings.supported_targets.size(); i++) {
            if(target.ratio_num != settings.supported_targets[i].ratio_num || target.ratio_den != settings.supported_targets[i].ratio_den)
                continue;

            if(target.also_encode[also_encode_i] == settings.supported_targets[i].height) {
                target.also_encode[also_encode_i] = i;

                if(++also_encode_i == target.also_encode.size())
                    break;
            }
        }

        if(also_encode_i < target.also_encode.size()) {
            LOG_ERROR << "also_encode resolution not defined for " << target.width << 'x' << target.height;
            return 1;
        }
    }

    std::string ffmpeg_log = custom_user_config.get("ffmpeg_log", "").as<std::string>();
    int ffmpeg_log_level = custom_user_config.get("ffmpeg_log_level", 32).as<int>();

    if(!ffmpeg_log.empty())
        VideoTranscoder::redirectLog(ffmpeg_log, ffmpeg_log_level);

    parseLimits(custom_user_config, settings.limits);
    return 0;
}

int main() {
    Settings& settings = Settings::getInstance();
    app().loadConfigFile("data/config.json");

    if(parseSettings(settings) != 0)
        return 1;

    HttpViewData data = Settings::getInstance().getData();
    data.insert("error-text", "404 Not Found");
    data.insert("back", "/");

    const drogon::HttpResponsePtr
        res404 = HttpResponse::newHttpViewResponse("error", std::move(data));

    app().getLoop()->runEvery(3'600, cleanupVideos);

    app().setSSLConfigCommands({
            { "Options", "Compression" }
        });

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
        .setClientMaxMemoryBodySize(10'485'760);

    app()
        .setMaxConnectionNum(10'000)
        .setMaxConnectionNumPerIP(10);

    app().run();
    VideoTranscoder transcoder("data/blobs/123456", "data/videos/f38d", {
            VideoTarget{
                .width = 1920,
                .height = 1080,
                .rate = {30, 1},
            }
        });
    return 0;
}
