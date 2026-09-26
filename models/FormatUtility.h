#pragma once
#include <utility>
#include <cstdint>
#include <array>
#include <string>

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

    namespace NumberScaleConstants {
        constexpr uint64_t
            hundred = 1.00e2,
            thousand = 1.00e3,
            million = 1.00e6,
            billion = 1.00e9;
    };

    class FormatUtil {
        static constexpr std::array<std::pair<int64_t, char>, 6> timeUnits {{
                { TimeConstants::year,   'y' },
                { TimeConstants::month,  'M' },
                { TimeConstants::week,   'w' },
                { TimeConstants::day,    'd' },
                { TimeConstants::hour,   'h' },
                { TimeConstants::minute, 'm' },
            }};

        static constexpr std::array<std::pair<int64_t, char>, 3> viewCountUnits = {{
                { NumberScaleConstants::billion,  'B' },
                { NumberScaleConstants::million,  'M' },
                { NumberScaleConstants::thousand, 'k' },
            }};

        public:
        static std::string FormatVideoAge(int64_t uploadTime);
        static std::string FormatViewCount(int64_t viewCount);
    };
}
