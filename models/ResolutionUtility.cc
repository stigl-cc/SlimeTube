#include <algorithm>
#include <models/ResolutionUtility.h>
#include <models/VideoTypes.h>

std::vector<SlimyTube::VideoTarget> SlimyTube::ResolutionUtility::GetForVideo(resolution_t res) {
    const Settings& settings = Settings::getInstance();

    resolution_t horizontal = {
        std::max(res.w, res.h),
        std::min(res.w, res.h),
    };

    bool is_horizontal = res.w >= res.h;

    // check limits
    if(res.w > settings.limits.max_input.w)
        throw video_proc_error(true, "resolution too big");
    if(res.h > settings.limits.max_input.h)
        throw video_proc_error(true, "resolution too big");
    if(static_cast<float>(horizontal.w) / horizontal.h >= settings.limits.aspectRatio)
        throw video_proc_error(true, "bad aspect ratio");

    // find aspect ratio
    std::pair<int, int> selected_ratio = { 0, 0 };
    for(const std::pair<int, int>& ratio : settings.supported_ratios) {
        if(horizontal.w * ratio.second == horizontal.h * ratio.first) {
            selected_ratio = ratio;
            break;
        }
    }

    std::vector<VideoTarget> result = {};
    if(res.w <= settings.limits.max_encode.w && res.h <= settings.limits.max_encode.h) {
        result.emplace_back(res.w, res.h);
    }

    // aspect ratio unsupported
    if(selected_ratio.first == 0) {
        if(!settings.allow_unlisted_ratios)
            throw video_proc_error(true, "bad aspect ratio");
        return result;
    }

    SupportedTarget selected_target = {};
    for(const SupportedTarget& target : settings.supported_targets) {
        if(target.width >= horizontal.w) {
            selected_target = target;
            break;
        }
    }

    // aspect ratio supported, smaller than the smallest resolution of it
    if(!selected_target.width) {
        return result;
    }

    result.clear();

    result.emplace_back(
        selected_target.width,
        selected_target.height
    );

    for(size_t also_encode : selected_target.also_encode) {
        const SupportedTarget& also_target = settings.supported_targets[also_encode];
        result.emplace_back(
            also_target.width,
            also_target.height
        );
    }
    return result;
}
