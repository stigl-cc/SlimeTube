#pragma once

#include <cstdint>
#include <string>

namespace SlimyTube {
    class Settings {
        public:
        std::string title;
        uint64_t zeroRangeLimit;

        static inline Settings& getInstance() {
            static Settings setting = {};
            return setting;
        }

        private:
        Settings() = default;

        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        Settings(Settings&&) noexcept = delete;
        Settings& operator=(Settings&&) noexcept = delete;
    };
}
