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
        keyUserId = "userId",
        keyLoginMethod = "loginMethod";
};
