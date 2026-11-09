#pragma once
#include "Videos.h"
#include <models/Settings.h>
#include <models/video/VideoTypes.h>

using drogon_model::SlimyTube::Videos;
namespace SlimyTube {
    class ResolutionUtility {
        public:
        static std::vector<VideoTarget> GetForVideo(resolution_t, int bitrate);

        static std::pair<int, int> GetRatio(resolution_t, const Settings& settings);
        static void CheckLimits(resolution_t res, resolution_t horizontal, const Settings& settings);
        static std::vector<uint16_t> GetResolutions(const Videos& video);
    };
};
