#pragma once
#include <models/VideoDecoder.h>
#include <models/VideoEncoder.h>

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

        public:
        VideoTranscoder(const std::string& inpath, const std::string& outpath, const std::vector<VideoTarget>& targets);
        void transcode();
    };
};
