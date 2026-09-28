#pragma once
#include <memory>
#include <optional>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdexcept>

namespace SlimyTube {
    class VideoDecoder {
        public:
        class error : public std::runtime_error {
            public:
            error(const char* const message) throw();
        };
        private:
        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        typedef std::unique_ptr<const AVCodec> codec_ptr_t;

        struct CodecContextDeleter { void operator()(AVCodecContext*) const; };
        typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

        struct PacketDeleter {void operator()(AVPacket*) const; };
        typedef std::unique_ptr<AVPacket, PacketDeleter> packet_ptr_t;

        typedef std::shared_ptr<AVFrame> frame_ptr_t;

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

        public:
        VideoDecoder(const std::string& path);
        std::optional<std::weak_ptr<AVFrame>> getFrame();
    };

    class VideoEncoder {
        struct OutputFormatContextDeleter { void operator()(AVFormatContext*); };

        std::unique_ptr<AVFormatContext, OutputFormatContextDeleter> outputFormatContext = nullptr;
    };

};
