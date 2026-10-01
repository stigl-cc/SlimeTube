#pragma once
#include <memory>
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

        class user_error : public std::runtime_error {
            public:
            user_error(const char* const message) throw();
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
        const AVCodec
            *mAudioCodec,
            *mVideoCodec;
        codec_cxt_ptr_t
            mAudioCodecContext,
            mVideoCodecContext;

        packet_ptr_t mPacket;
        frame_ptr_t mFrame;

        bool mGetFramePacketAvail = false;
        codec_cxt_ptr_t* mCurrentCodecUsed = nullptr;

        void setupCodec(const AVStream* stream, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);
        void readBlob(const std::string& path);
        void selectStreams();

        bool getFramePacketAvail();

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
        static constexpr const std::string_view
            format_name = "mp4";

        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        struct StreamDeleter { void operator()(const AVStream*) const; };
        typedef std::unique_ptr<AVStream, StreamDeleter> stream_ptr_t;

        format_cxt_ptr_t mFormatContext;

        const AVCodec
            *mAudioCodec,
            *mVideoCodec;
        codec_cxt_ptr_t
            mAudioCodecContext,
            mVideoCodecContext;

        stream_ptr_t
            videoStream,
            audioStream;

        void openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams);
        void setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);

        public:
        VideoEncoder(const std::string& path, int width, int height, int rate, AVCodecID videoCodec = AV_CODEC_ID_H264, AVCodecID audioCodec = AV_CODEC_ID_AAC);


    };

    class VideoTranscoder {
        public:
        VideoDecoder mDecoder;
        VideoEncoder mEncoder;
    };
};
