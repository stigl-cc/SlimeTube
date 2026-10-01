#pragma once
#include <memory>
#include <optional>
#include <stdexcept>
#include <functional>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace SlimyTube {
    class VideoDecoder;
    class VideoEncoder;

    struct CodecDeleter { void operator()(const AVCodec*) const; };
    typedef std::unique_ptr<const AVCodec, CodecDeleter> codec_ptr_t;

    struct CodecContextDeleter { void operator()(AVCodecContext*) const; };
    typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

    struct PacketDeleter {void operator()(AVPacket*) const; };
    typedef std::unique_ptr<AVPacket, PacketDeleter> packet_ptr_t;

    typedef std::shared_ptr<AVFrame> frame_ptr_t;
    typedef std::weak_ptr<AVFrame> weak_frame_ptr_t;

    typedef std::function<void(weak_frame_ptr_t frame)> frame_callback_t;

    class VideoDecoder {
        public:
        class error : public std::runtime_error {
            public:
            error(const char* const message) throw();
        };

        private:

        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        private:
        int
            mAudioStreamIndex = -1,
            mVideoStreamIndex = -1;

        format_cxt_ptr_t
            mFormatContext = nullptr;
        codec_ptr_t
            mAudioCodec,
            mVideoCodec;
        codec_cxt_ptr_t
            mAudioCodecContext,
            mVideoCodecContext;

        packet_ptr_t mPacket;
        frame_ptr_t mFrame;

        bool mGetFramePacketAvail = false;

        void setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext);
        void readBlob(const std::string& path);
        void selectStreams();
        bool decodeFrame();

        public:
        VideoDecoder(const std::string& path);

        void getFrames(frame_callback_t callback);
        weak_frame_ptr_t getFrame();
    };

    class VideoEncoder {
        public:
        class error : public std::runtime_error {
            public:
            error(const char* const message) throw();
        };

        private:
        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;


    };

    class VideoTranscoder {
        public:
        VideoDecoder mDecoder;
        VideoEncoder mEncoder;
    };

};
