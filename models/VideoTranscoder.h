#pragma once
#include <deque>
#include <fstream>
#include <models/VideoTypes.h>
#include <models/VideoDecoder.h>
#include <models/VideoEncoder.h>
#include <mutex>

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

        std::string mOutPath;

        public:
        std::vector<VideoTarget> mTargets;

        VideoTranscoder(const std::string& inpath, const std::string& outpath, const std::vector<VideoTarget>& targets = {});
        void transcode();
        const VideoDecoder& getDecoder() const;

        static void redirectLog(const std::string& path, int logLevel);
    };

    class TranscodeManager {
        protected:
        std::mutex mJobsMutex;
        std::deque<std::string> mJobs;
        void worker();

        public:
        TranscodeManager();
        void enqueueJob(const std::string& id);
    };
};
