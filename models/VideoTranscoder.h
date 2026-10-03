#pragma once
#include "VideoTypes.h"
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

        void onVideoFrame(weak_frame_ptr_t frame);
        void onAudioFrame(weak_frame_ptr_t frame);

        public:
        VideoTranscoder(const std::string& inpath, const std::string& outpath, const std::vector<VideoTarget>& targets);
        void transcode();
    };
};
