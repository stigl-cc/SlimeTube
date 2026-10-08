#pragma once
#include <models/Settings.hh>
#include <models/VideoTypes.h>

namespace SlimyTube {
    class ResolutionUtility {
        public:
        static void CheckLimits(resolution_t res);
        static std::vector<VideoTarget> GetForVideo(resolution_t);
    };
};
