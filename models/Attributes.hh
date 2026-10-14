#pragma once
#include <cstdint>
#include <string_view>

namespace SlimyTube::Attributes {
    enum struct LoginMethod {
        LaxNone = 0,
        Session,
        Password,
    };

    typedef uint32_t userId_t;
    constexpr const std::string_view
        userId = "userId",
        keyLoginMethod = "loginMethod";
};

namespace SlimyTube::Session {
    constexpr const std::string_view
        userId = "userId";
};

namespace SlimyTube::Query {
    static constexpr std::string_view
        username = "name",
        password = "pass";

    static constexpr std::string_view
        newPassword = "new_pass",
        newUsername = "new_user";

    static constexpr std::string_view
        videoTitle = "title",
        videoDescription = "desc";

    static constexpr std::string_view
        videoQuality = "quality",
        videoSpeed = "speed";
};

namespace SlimyTube::Headers {
    static constexpr std::string_view
        location = "Location",
        range = "Range";

    static constexpr std::string_view
        position = "X-Position",
        last = "X-Last",
        offset = "X-Offset";
};
