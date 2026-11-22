#pragma once
#include <cstdint>
#include <string>

namespace SlimyTube {
    struct Preview {
        std::string id;
        std::string length;
        std::string title;
        std::string channel;
        std::string views;
        std::string age;
    };

    struct Channel {
        int64_t id;
        std::string name;
        std::string total_views;
        int64_t total_videos;
    };

    struct Comment {
        uint64_t channel_id;
        std::string channel_name;
        std::string age;
        std::string text;
    };
};
