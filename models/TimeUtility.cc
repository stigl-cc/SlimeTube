#include <TimeUtility.h>
#include <chrono>

using namespace std::chrono;

int64_t SlimyTube::TimeUtil::GetUnixSecond() {
    return time_point_cast<std::chrono::seconds>(system_clock::now()).time_since_epoch().count();
}
