#include "utils/TimeUtility.h"
#include <models/utils/FormatUtility.h>
#include <iomanip>
#include <charconv>
#include <stdexcept>

namespace SlimyTube {
    std::string FormatUtil::FormatVideoAge(int64_t uploadTime) {
        const int64_t
            current_time = TimeUtil::GetUnixSecond(),
            video_age = current_time - uploadTime;

        constexpr size_t age_str_size = 5; // maximum of 9999years... propably enough
        char age_str[age_str_size] = {};

        std::pair<int64_t, char> used_unit = { 1, 's' };

        // find appropriate unit
        for(const std::pair<int64_t, char>& time_unit : timeUnits) {
            if(video_age >= time_unit.first) {
                used_unit = time_unit;
                break;
            }
        }

        // convert to string
        std::to_chars_result result = std::to_chars(age_str, age_str + age_str_size - 1, static_cast<int64_t>(video_age / used_unit.first));
        if(result.ec != std::errc())
            throw std::runtime_error("Failed to convert video age: " +  std::make_error_code(result.ec).message());

        // add unit mark
        *result.ptr++ = used_unit.second;
        return std::string(age_str, result.ptr);
    }

    std::string FormatUtil::FormatViewCount(int64_t viewCount) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);

        std::pair<int64_t, char> used_unit = { 1, 0 };

        // find appropriate unit
        for(const std::pair<int64_t, char>& view_count_unit : viewCountUnits) {
            if(viewCount >= view_count_unit.first) {
                used_unit = view_count_unit;
                break;
            }
        }

        if(used_unit.first == 1)
            ss << std::setprecision(0);

        ss << viewCount / static_cast<float>(used_unit.first);
        if(used_unit.second) ss << used_unit.second;

        return ss.str();
    }

    std::string FormatUtil::FormatVideoLength(int64_t length) {
        std::stringstream ss;

        if(length <= 0)
            return std::string();

        const int32_t
            seconds = length % TimeConstants::minute,
            minutes = static_cast<int32_t>(length / TimeConstants::minute),
            hours = static_cast<int32_t>(length / TimeConstants::hour);

        if(hours)
            ss << hours << ':';

        ss << std::setw(2)
           << std::setfill('0')
           << minutes << ':'
           << std::setw(2)
           << seconds;

        return ss.str();
    }
};
