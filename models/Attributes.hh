#pragma once
#include <string_view>

namespace SlimyTube::Attributes {
    enum struct LoginMethod {
        Session,
        Password,
    };

    constexpr const std::string_view
        keyUserId = "userId",
        keyLoginMethod = "loginMethod";
};
