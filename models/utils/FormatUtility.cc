#include <charconv>
#include <iomanip>
#include <models/utils/FormatUtility.h>
#include <sstream>
#include <stdexcept>

std::string SlimyTube::FormatUtil::FormatVideoAge(int64_t uploadTime) {
  const int64_t current_time = TimeUtil::GetUnixSecond(),
                video_age = current_time - uploadTime;

  constexpr size_t age_str_size = 5; // maximum of 9999years... propably enough
  char age_str[age_str_size] = {};

  std::pair<int64_t, char> used_unit = {1, 's'};

  // find appropriate unit
  for (const std::pair<int64_t, char> &time_unit : timeUnits) {
    if (video_age >= time_unit.first) {
      used_unit = time_unit;
      break;
    }
  }

  // convert to string
  std::to_chars_result result =
      std::to_chars(age_str, age_str + age_str_size - 1,
                    static_cast<int64_t>(video_age / used_unit.first));
  if (result.ec != std::errc())
    throw std::runtime_error("Failed to convert video age: " +
                             std::make_error_code(result.ec).message());

  // add unit mark
  *result.ptr++ = used_unit.second;
  return std::string(age_str, result.ptr);
}

std::string SlimyTube::FormatUtil::FormatViewCount(int64_t viewCount) {
  std::stringstream ss;
  ss << std::fixed << std::setprecision(1);

  std::pair<int64_t, char> used_unit = {1, 0};

  // find appropriate unit
  for (const std::pair<int64_t, char> &view_count_unit : viewCountUnits) {
    if (viewCount >= view_count_unit.first) {
      used_unit = view_count_unit;
      break;
    }
  }

  if (used_unit.first == 1)
    ss << std::setprecision(0);

  ss << viewCount / static_cast<float>(used_unit.first);
  if (used_unit.second)
    ss << used_unit.second;

  return ss.str();
}

std::string SlimyTube::FormatUtil::FormatVideoLength(int64_t durationMs) {
  if (durationMs < 0)
    return "";

  const int64_t totalSeconds = durationMs / TimeConstants::minute;
  const int64_t hours = totalSeconds / TimeConstants::hour;
  const int64_t minutes = (totalSeconds / TimeConstants::minute) % TimeConstants::minute;
  const int64_t seconds = durationMs % TimeConstants::minute;

  std::stringstream ss;

  ss << std::setfill('0');

  if (hours > 0)
    ss << hours << ":" << std::setw(2) << minutes;
  else
    ss << minutes;

  ss << ':' << std::setw(2) << seconds;
  return ss.str();
}
