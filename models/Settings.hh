#pragma once

#include "VideoTypes.h"
#include <drogon/HttpViewData.h>
#include <filesystem>
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
        std::filesystem::path
            thumbnails_dir,
            videos_dir,
            blobs_dir;

        size_t salt_len;
        Limits limits;

        std::vector<SupportedTarget> supported_targets;
        std::set<std::pair<int, int>> supported_ratios;
        bool allow_unlisted_ratios;
        int default_unlisted_framerate;

        static inline Settings& getInstance() {
            static Settings setting = {};
            return setting;
        }

        inline drogon::HttpViewData getData() {
            drogon::HttpViewData data;
            data.insert("title", title);
            return data;
        }

        private:
        Settings() = default;

        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        Settings(Settings&&) noexcept = delete;
        Settings& operator=(Settings&&) noexcept = delete;
    };
}
