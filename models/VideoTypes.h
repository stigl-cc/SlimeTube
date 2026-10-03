#pragma once
#include <libavutil/rational.h>
#include <memory>
#include <functional>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace SlimyTube {
    struct CodecContextDeleter { void operator()(AVCodecContext*) const; };
    typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

    struct PacketDeleter {void operator()(AVPacket*) const; };
    typedef std::unique_ptr<AVPacket, PacketDeleter> packet_ptr_t;

    typedef std::shared_ptr<AVFrame> frame_ptr_t;
    typedef std::weak_ptr<AVFrame> weak_frame_ptr_t;

    typedef std::function<void(weak_frame_ptr_t frame)> video_callback_t;
    typedef std::function<void(weak_frame_ptr_t frame)> audio_callback_t;

    struct VideoTarget {
        int
            width,
            height;
        AVRational rate;
        AVCodecID
            videoCodec = AV_CODEC_ID_H264,
            audioCodec = AV_CODEC_ID_AAC;
    };
}
