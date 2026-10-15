#include <models/Settings.hh>
#include <models/VideoTranscoder.h>
#include <models/VideoTypes.h>
#include <filesystem>

namespace fs = std::filesystem;

bool ensureDirectory(fs::path p) {
    std::error_code ec;
    std::filesystem::file_status status = fs::status(p, ec);

    if (!fs::exists(p)) {
        fs::create_directory(p, ec);
        if(ec != std::errc())
            LOG_ERROR << "Failed to create directory " << fs::weakly_canonical(p) << ": " << ec.message();
    } else if (!fs::is_directory(status)) {
        LOG_ERROR << fs::canonical(p) << ": path exists but is not a directory.";
        return false;
    }
    return true;
}

void SlimyTube::Settings::parseLimits(Json::Value config, SlimyTube::Limits& limits) {
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
    if(std::fabs(limits.aspectRatio + 1) <= std::numeric_limits<float>::epsilon()) // handles 0.00000
        limits.aspectRatio = std::numeric_limits<float>::max();

    limits.max_input.w = user_limit_config.get("max_input_video_width", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
    limits.max_input.h = user_limit_config.get("max_input_video_height", std::numeric_limits<dimension_t>::max()).as<dimension_t>();

    limits.max_encode.w = user_limit_config.get("max_encode_video_width", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
    limits.max_encode.h = user_limit_config.get("max_encode_video_height", std::numeric_limits<dimension_t>::max()).as<dimension_t>();
    limits.max_unlisted_br = user_limit_config.get("max_unlisted_bitrate", 3150000).as<int32_t>();
    if(limits.max_unlisted_br == -1) // 2gbps ish at the maximum per second of video - likely fine
        limits.max_unlisted_br = std::numeric_limits<int>::max();
}

int SlimyTube::Settings::parse(Json::Value config) {
    title = config.get("title", "SlimyTube").as<std::string>();
    salt_len = config.get("salt_len", 16).as<size_t>();

    thumbnails_dir = config.get("thumbnails_dir", "data/thumbnails").as<std::string>();
    if(!ensureDirectory(thumbnails_dir))
        return 1;

    videos_dir = config.get("videos_dir", "data/videos").as<std::string>();
    if(!ensureDirectory(videos_dir))
        return 1;

    blobs_dir = config.get("blobs_dir", "data/blobs").as<std::string>();
    if(!ensureDirectory(blobs_dir))
        return 1;

    encoder_frame_rate = config.get("default_unlisted_framerate", 30).as<int>();
    allow_unlisted_ratios = config.get("allow_unlisted_ratios", false).as<bool>();

    audio_bitrate = config.get("audio_bitrate", 128000).as<int>();
    audio_samprate = config.get("audio_sample_rate", 48000).as<int>();

    for(const Json::Value& resolution : config.get("resolutions", {})) {
        SupportedTarget target;
        try {
            if(resolution.size() < 5)
                throw Json::LogicError("config resolution format not recognized, please use [[ratio_n, ratio_d], w, h, br, also_encode],");
            target.ratio_num = resolution[0][0].asInt();
            target.ratio_den = resolution[0][1].asInt();

            supported_ratios.emplace(target.ratio_num, target.ratio_den);

            target.width = resolution[1].asInt();
            target.height = resolution[2].asInt();

            target.max_bitrate = resolution[3].asInt();
            for(const Json::Value& also : resolution[4])
                target.also_encode.push_back(also.asInt());
            supported_targets.push_back(target);
        } catch(const Json::LogicError& ex) {
            LOG_ERROR << "Error while processing resolution " << resolution.toStyledString() << "" << ex.what();
            return 1;
        }
    }

    // convert also_encode heights to their indexes (O(n^2) - in practice O(n))
    for(int n =0; n < supported_targets.size(); n++) {
        SupportedTarget& target = supported_targets[n];

        int also_encode_i = 0;
        if(target.also_encode.empty())
            continue;

        for(int i =n; i < supported_targets.size(); i++) {
            if(target.ratio_num != supported_targets[i].ratio_num || target.ratio_den != supported_targets[i].ratio_den)
                continue;

            if(target.also_encode[also_encode_i] == supported_targets[i].height) {
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

    std::string ffmpeg_log = config.get("ffmpeg_log", "").as<std::string>();
    int ffmpeg_log_level = config.get("ffmpeg_log_level", 32).as<int>();

    if(!ffmpeg_log.empty())
        VideoTranscoder::redirectLog(ffmpeg_log, ffmpeg_log_level);

    parseLimits(config, limits);
    return 0;
}
