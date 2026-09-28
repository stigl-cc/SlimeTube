#pragma once
#include <memory>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

namespace SlimyTube {
    class VideoDecoder {
        private:
        struct FormatContextDeleter { void operator()(AVFormatContext*); };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        struct CodecDeleter { void operator()(const AVCodec*); };
        typedef std::unique_ptr<const AVCodec, CodecDeleter> codec_ptr_t;

        struct CodecContextDeleter { void operator()(AVCodecContext*); };
        typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

        struct PacketDeleter {void operator()(AVPacket*); };
        typedef std::unique_ptr<AVPacket, PacketDeleter> packet_ptr_t;

        struct FrameDeleter {void operator()(AVFrame*); };
        typedef std::unique_ptr<AVFrame, FrameDeleter> frame_ptr_t;

        private:
        int
            mAudioStreamIndex = -1,
            mVideoStreamIndex = -1;

        format_cxt_ptr_t mFormatContext = nullptr;

        codec_ptr_t
            mAudioCodec,
            mVideoCodec;

        codec_cxt_ptr_t
            mAudioCodecContext,
            mVideoCodecContext;

        packet_ptr_t mPacket;
        frame_ptr_t mFrame;

        void setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext);

        void readBlob(const std::string& path);

        void selectStreams();
        void decode();

        public:
        VideoDecoder();
    };

    class VideoEncoder {
        struct OutputFormatContextDeleter { void operator()(AVFormatContext*); };

        std::unique_ptr<AVFormatContext, OutputFormatContextDeleter> outputFormatContext = nullptr;
    };

};
