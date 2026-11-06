#pragma once

#include <array>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

namespace SlimyTube {
    class ViewCountManager {
        protected:
        struct Stripe {
            std::mutex mutex;
            std::set<std::string> videos;
        };

        static constexpr size_t mutexCount = 128;
        std::array<Stripe, mutexCount> videoMutexes;
        std::unordered_map<std::string, uint32_t> mViewQueue;
        std::string mCachePath;

        void restoreState();
        void saveState();

        ViewCountManager();

        public:
        ViewCountManager(ViewCountManager&&) = delete;
        ViewCountManager(const ViewCountManager&) = delete;

        ViewCountManager& operator=(ViewCountManager&&) = delete;
        ViewCountManager& operator=(const ViewCountManager&) = delete;

        ~ViewCountManager();

        void processViews();
        void addView(const std::string& id, uint32_t amount = 1);
        void setCachePath(std::string cachePath);

        static ViewCountManager& getInstance();
    };
};
