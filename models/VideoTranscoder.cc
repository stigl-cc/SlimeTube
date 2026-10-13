#include "ResolutionUtility.h"
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
#include <thread>

extern "C" {
#include <libavutil/log.h>
#include <libavcodec/codec.h>
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

const SlimyTube::VideoDecoder& SlimyTube::VideoTranscoder::getDecoder() const {
    return mDecoder;
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
    : mJobs(),
      worker_thread(std::thread(std::bind(&TranscodeManager::worker, this))) {
}

SlimyTube::TranscodeManager::~TranscodeManager() {
    if(worker_thread.joinable())
        worker_thread.join();
}

void SlimyTube::TranscodeManager::worker() {
    while(mJobs.empty());

    std::lock_guard<std::mutex> lk(mJobsMutex);
    if(!mJobs.empty()) {
        std::string current_job = mJobs.front();
        try {
            std::string
                in_path = Settings::getInstance().blobs_dir / current_job,
                out_path = Settings::getInstance().videos_dir / current_job;

            VideoTranscoder transcoder(in_path, out_path);

            int width, height, bit_rate;
            transcoder.getDecoder().getMetaData(width, height, bit_rate);
            transcoder.mTargets = std::move(ResolutionUtility::GetForVideo({width, height}, bit_rate));

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
