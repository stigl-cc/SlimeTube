#pragma once
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <models/video/VideoTypes.h>
#include <models/video/VideoDecoder.h>
#include <models/video/VideoEncoder.h>
#include <mutex>
#include <thread>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace SlimyTube {
    class VideoTranscoder {
        private:
        VideoDecoder mDecoder;
        std::vector<VideoEncoder> mEncoders;

        void onVideoFrame(weak_frame_ptr_t frame);
        void onAudioFrame(weak_frame_ptr_t frame);

        // For ffmpeg log redirection
        static char logBuffer[2048];
        static std::mutex logMutex;
        static int logLevel;
        static std::ofstream logFileStream;
        static void log(void*, int, const char*, va_list);

        std::filesystem::path mOutPath;

        public:
        std::vector<VideoTarget> mTargets;

        VideoTranscoder(const std::filesystem::path& inpath, const std::filesystem::path& outpath, const std::vector<VideoTarget>& targets = {});
        void transcode();
        const VideoDecoder& getDecoder() const;

        static void redirectLog(const std::string& path, int logLevel);
    };

    class TranscodeManager {
        protected:
        std::condition_variable mConditionVariable;
        std::mutex mJobsMutex;
        std::thread mWorkerThread;
        bool shouldRun;

        std::deque<std::string> mJobs;
        void worker();

        TranscodeManager();

        public:
        TranscodeManager(TranscodeManager&&) = delete;
        TranscodeManager(const TranscodeManager&) = delete;

        TranscodeManager& operator=(TranscodeManager&&) = delete;
        TranscodeManager& operator=(const TranscodeManager&) = delete;

        ~TranscodeManager();
        void enqueueJob(const std::string& id);

        static TranscodeManager& getInstance();
    };
};
