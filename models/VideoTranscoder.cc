#include <models/VideoTranscoder.h>
#include <models/VideoDecoder.h>
#include <models/VideoEncoder.h>
#include <models/VideoTypes.h>

#include <functional>
#include <format>
#include <mutex>
#include <stdexcept>

extern "C" {
#include <libavutil/log.h>
}

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

char SlimyTube::VideoTranscoder::logBuffer[2048] = {};
std::mutex SlimyTube::VideoTranscoder::logMutex = {};
std::ofstream SlimyTube::VideoTranscoder::logFileStream = {};
int SlimyTube::VideoTranscoder::logLevel = AV_LOG_INFO;

void SlimyTube::VideoTranscoder::log(void*, int level, const char* msg, va_list va) {
    if(level <= logLevel) {
        std::lock_guard lk(logMutex);
        int len = vsnprintf(logBuffer, sizeof(logBuffer), msg, va);
        logFileStream << logBuffer;
    }
}

void SlimyTube::VideoTranscoder::redirectLog(const std::string& path, int logLevel) {
    std::lock_guard lk(logMutex);

    logFileStream = std::ofstream(path.c_str(), std::ios::app | std::ios::out);
    if(!logFileStream.is_open())
        throw std::runtime_error("Could not open ffmpeg log file!");

    av_log_set_level(logLevel);
    av_log_set_callback(&VideoTranscoder::log);
}
