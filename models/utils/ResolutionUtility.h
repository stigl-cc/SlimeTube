#pragma once
#include <models/Settings.hh>
#include <models/video/VideoTypes.h>

namespace SlimyTube {
    class ResolutionUtility {
        public:
        static std::vector<VideoTarget> GetForVideo(resolution_t, int bitrate);

        static std::pair<int, int> GetRatio(resolution_t, const Settings& settings);
        static void CheckLimits(resolution_t res, resolution_t horizontal, const Settings& settings);
    };
};
