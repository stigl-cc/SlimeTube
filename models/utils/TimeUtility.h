#pragma once
#include <cstdint>

namespace SlimyTube {
    namespace TimeConstants {
        constexpr uint64_t
            minute =        60,
            hour =       3'600,
            day =       86'400,
            week =     604'800,
            month =  2'628'000,
            year =  31'556'952;
    };

    class TimeUtil {
        public:
        static int64_t GetUnixSecond();
    };
};
