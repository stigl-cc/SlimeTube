#pragma once

#include <models/video/VideoTypes.h>
#include <drogon/HttpViewData.h>
#include <filesystem>
#include <json/value.h>
#include <set>
#include <string>

namespace SlimyTube {
    class Limits {
        public:
        ssize_t
            blobSize,
            totalUserSize,
            getVideoMaxRange;
        float
            aspectRatio;

        resolution_t max_input;
        resolution_t max_encode;
        int max_unlisted_br;
    };

    class Settings {
        public:
        std::string title;
        std::filesystem::path videos_dir;

        size_t salt_len;
        Limits limits;

        std::vector<SupportedTarget> supported_targets;
        std::set<std::pair<int, int>> supported_ratios;
        bool allow_unlisted_ratios;

        int audio_bitrate;
        int audio_samprate;

        int encoder_frame_rate;

        static inline Settings& getInstance() {
            static Settings setting = {};
            return setting;
        }

        inline drogon::HttpViewData getData() const {
            drogon::HttpViewData data;
            data.insert("title", title);
            return data;
        }

        void parseLimits(Json::Value config, SlimyTube::Limits& limits);
        int parse(Json::Value config);

        private:
        Settings() = default;

        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        Settings(Settings&&) noexcept = delete;
        Settings& operator=(Settings&&) noexcept = delete;
    };
}
