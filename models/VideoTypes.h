#pragma once
#include <libavutil/rational.h>
#include <memory>
#include <functional>
#include <stdexcept>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace SlimyTube {
    class video_proc_error : public std::runtime_error {
        public:
        bool isUserCaused;
        video_proc_error(bool isUserCaused, const char* const message) throw();
    };

    struct CodecContextDeleter { void operator()(AVCodecContext*) const; };
    typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

    struct PacketDeleter {void operator()(AVPacket*) const; };
    typedef std::unique_ptr<AVPacket, PacketDeleter> packet_ptr_t;

    typedef std::shared_ptr<AVFrame> frame_ptr_t;
    typedef std::weak_ptr<AVFrame> weak_frame_ptr_t;

    typedef std::function<void(weak_frame_ptr_t frame)> video_callback_t;
    typedef std::function<void(weak_frame_ptr_t frame)> audio_callback_t;

    typedef int dimension_t;
    struct resolution_t { dimension_t w, h; };

    struct VideoTarget {
        int
            width,
            height;
        AVRational rate;
        AVCodecID
            videoCodec = AV_CODEC_ID_H264,
            audioCodec = AV_CODEC_ID_AAC;
    };

    class SupportedTarget {
        public:
        int ratio_num, ratio_den;
        int width, height;
        int
            max_framerate,
            max_bitrate;
        std::vector<size_t> also_encode;
    };
}
