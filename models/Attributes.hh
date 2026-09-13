#pragma once
#include <string_view>

namespace SlimyTube::Attributes {
    enum struct LoginMethod {
        LaxNone = 0,
        Session,
        Password,
    };

    constexpr const std::string_view
        keyUserId = "userId",
        keyLoginMethod = "loginMethod";
};
