#include <models/VideoTranscoder.h>
#include <models/VideoDecoder.h>
#include <models/VideoEncoder.h>
#include <models/VideoTypes.h>

#include <functional>
#include <format>

void SlimyTube::VideoTranscoder::onVideoFrame(weak_frame_ptr_t weak_frame) {
    frame_ptr_t frame_ptr = weak_frame.lock();
    if(!frame_ptr)
        throw video_proc_error(false, "frame expired");

    for(VideoEncoder& encoder : mEncoders)
        encoder.putVideoFrame(frame_ptr);
}

void SlimyTube::VideoTranscoder::onAudioFrame(weak_frame_ptr_t weak_frame) {
    frame_ptr_t frame_ptr = weak_frame.lock();
    if(!frame_ptr)
        throw video_proc_error(false, "frame expired");
    for(VideoEncoder& encoder : mEncoders)
        encoder.putAudioFrame(frame_ptr);
}

SlimyTube::VideoTranscoder::VideoTranscoder(const std::string& inpath, const std::string& outpath, const std::vector<VideoTarget>& targets)
    : mDecoder(inpath),
      mEncoders() {
    AVRational video, audio;
    mDecoder.getTimeBase(video, audio);
    for(const VideoTarget& target : targets)
        mEncoders.emplace_back(std::format("{}-{}", outpath, target.height), target, video, audio);
}

void SlimyTube::VideoTranscoder::transcode() {
    mDecoder.getFrames(
        std::bind(&VideoTranscoder::onVideoFrame, this, std::placeholders::_1),
        std::bind(&VideoTranscoder::onAudioFrame, this, std::placeholders::_1)
    );

    for(VideoEncoder& encoder : mEncoders)
        encoder.finish();
}
