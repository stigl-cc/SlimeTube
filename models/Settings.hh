#pragma once

#include <drogon/HttpViewData.h>
#include <string>

using drogon::HttpViewData;

namespace SlimyTube {
    class Settings {
        public:
        std::string title;
        size_t zeroRangeLimit;
        std::string
            thumbnails_dir,
            videos_dir;
        size_t salt_len;

        static inline Settings& getInstance() {
            static Settings setting = {};
            return setting;
        }

        inline HttpViewData getData() {
            HttpViewData data;
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
