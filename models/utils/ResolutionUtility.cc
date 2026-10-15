#include <algorithm>
#include <models/utils/ResolutionUtility.h>

std::pair<int, int> SlimyTube::ResolutionUtility::GetRatio(resolution_t res, const Settings& settings) {
    // find aspect ratio
    std::pair<int, int> selected_ratio = { 0, 0 };
    for(const std::pair<int, int>& ratio : settings.supported_ratios) {
        if(res.w * ratio.second == res.h * ratio.first) {
            selected_ratio = ratio;
            break;
        }
    }
    return selected_ratio;
}

void SlimyTube::ResolutionUtility::CheckLimits(resolution_t res, resolution_t horizontal, const Settings& settings) {
    // check limits
    if(res.w > settings.limits.max_input.w)
        throw video_proc_error(true, "resolution too big");
    if(res.h > settings.limits.max_input.h)
        throw video_proc_error(true, "resolution too big");
    if(static_cast<float>(horizontal.w) / horizontal.h >= settings.limits.aspectRatio)
        throw video_proc_error(true, "bad aspect ratio");
}

std::vector<SlimyTube::VideoTarget> SlimyTube::ResolutionUtility::GetForVideo(resolution_t res, int bitrate) {
    const Settings& settings = Settings::getInstance();

    resolution_t horizontal = {
        std::max(res.w, res.h),
        std::min(res.w, res.h),
    };

    bool is_horizontal = res.w >= res.h;

    ResolutionUtility::CheckLimits(res, horizontal, settings);

    std::pair<int, int> selected_ratio = ResolutionUtility::GetRatio(horizontal, settings);

    std::vector<VideoTarget> result = {};
    // aspect ratio unsupported
    if(selected_ratio.first == 0) {
        if(!settings.allow_unlisted_ratios)
            throw video_proc_error(true, "bad aspect ratio");

        if(res.w <= settings.limits.max_encode.w && res.h <= settings.limits.max_encode.h) {
            result.emplace_back(res.w, res.h, std::min(settings.limits.max_unlisted_br, bitrate), settings.audio_bitrate, settings.audio_samprate);
        }
        return result;
    }

    // aspect ratio supported - find largest (smaller or equal) resolution
    SupportedTarget selected_target = {};
    for(const SupportedTarget& target : settings.supported_targets) {
        if(horizontal.w >= target.width) {
            selected_target = target;
            break;
        }
    }

    // smaller than the smallest resolution of ratio
    if(!selected_target.width) {
        result.emplace_back(res.w, res.h, std::min(settings.limits.max_unlisted_br, bitrate), settings.audio_bitrate, settings.audio_samprate);
        return result;
    }

    result.emplace_back(
        selected_target.width,
        selected_target.height,
        std::min(selected_target.max_bitrate, bitrate),
        settings.audio_bitrate,
        settings.audio_samprate
    );

    for(size_t also_encode : selected_target.also_encode) {
        const SupportedTarget& also_target = settings.supported_targets[also_encode];
        result.emplace_back(
            also_target.width,
            also_target.height,
            std::min(also_target.max_bitrate, bitrate),
            settings.audio_bitrate,
            settings.audio_samprate
        );
    }
    return result;
}
