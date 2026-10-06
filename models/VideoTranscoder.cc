#include "Settings.hh"
#include <filesystem>
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
      mEncoders(),
      mOutPath(outpath) {

    for(const VideoTarget& target : targets)
        mTargets.push_back(target);
}

void SlimyTube::VideoTranscoder::transcode() {
    AVRational video, audio;
    mDecoder.getTimeBase(video, audio);

    for(const VideoTarget& target : mTargets)
        mEncoders.emplace_back(std::format("{}-{}", mOutPath, target.height), target, video, audio);

    mDecoder.getFrames(
        std::bind(&VideoTranscoder::onVideoFrame, this, std::placeholders::_1),
        std::bind(&VideoTranscoder::onAudioFrame, this, std::placeholders::_1)
    );

    for(VideoEncoder& encoder : mEncoders)
        encoder.finish();
}

void SlimyTube::VideoTranscoder::assumeTargets() {
    int width, height, bit_rate;
    float ratio = std::fabs(static_cast<float>(width) / height);
    mDecoder.getMetaData(width, height, bit_rate);

    if(ratio >= Settings::getInstance().limits.aspectRatio)
        throw video_proc_error(true, "wrong aspect ratio");
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

SlimyTube::TranscodeManager::TranscodeManager()
    : mJobs() { }

void SlimyTube::TranscodeManager::worker() {
    std::lock_guard<std::mutex> lk(mJobsMutex);
    if(!mJobs.empty()) {
        try {
            std::string
                in_path = Settings::getInstance().blobs_dir / mJobs.front(),
                out_path = Settings::getInstance().videos_dir / mJobs.front();

            VideoTranscoder transcoder(in_path, out_path);
            transcoder.assumeTargets();
            transcoder.transcode();
        } catch(const video_proc_error& err) {

        }
        mJobs.pop_front();
    }
}

void SlimyTube::TranscodeManager::enqueueJob(const std::string& id) {
    std::lock_guard<std::mutex> lk(mJobsMutex);
    mJobs.emplace_back(id);
}
