#include <models/VideoTranscoder.h>
#include <format>

SlimyTube::VideoTranscoder::VideoTranscoder(const std::string& inpath, const std::string& outpath, const std::vector<VideoTarget>& targets) : mDecoder(inpath) {
    AVRational video, audio;
    mDecoder.getTimeBase(video, audio);
    for(const VideoTarget& target : targets) {
        mEncoders.emplace_back(std::format("{}-{}", outpath, target.height), target, video, audio);
    }
}

void SlimyTube::VideoTranscoder::transcode() {
    
}
