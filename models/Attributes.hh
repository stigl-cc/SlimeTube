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
        password = "pass",
        username = "user";

    static constexpr std::string_view
        newPassword = "new_pass",
        newUsername = "new_user";

    static constexpr std::string_view
        videoTitle = "title",
        videoDescription = "desc";
};

namespace SlimyTube::Headers {
    static constexpr std::string_view
        location = "Location";
};
