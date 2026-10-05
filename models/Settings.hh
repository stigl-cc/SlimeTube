#pragma once

#include <drogon/HttpViewData.h>
#include <filesystem>
#include <string>

namespace SlimyTube {
    class Limits {
        public:
        ssize_t
            blobSize,
            totalUserSize;
        ssize_t getVideoMaxRange;
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
