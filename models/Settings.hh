#pragma once

#include <drogon/HttpViewData.h>
#include <string>

namespace SlimyTube {
    class Settings {
        public:
        std::string title;
        size_t zeroRangeLimit;
        std::string
            thumbnails_dir,
            videos_dir,
            blobs_dir;
        size_t salt_len;

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
