#pragma once
#include <models/VideoTypes.h>

namespace SlimyTube {
    class VideoEncoder {
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
            mVideoStream,
            mAudioStream;

        AVRational
            mAudioSourceTimeBase,
            mVideoSourceTimeBase;

        void openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams);
        void setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);

        public:
        VideoEncoder(
            const std::string& path,
            const VideoTarget& target,
            AVRational videoTimeBase,
            AVRational audioTimeBase);

        VideoEncoder(VideoEncoder&&);
        VideoEncoder(const VideoEncoder&) = delete;

        VideoEncoder& operator=(VideoEncoder&&);
        VideoEncoder& operator=(const VideoEncoder&) = delete;

        void putVideoFrame(frame_ptr_t& frame);
        void putAudioFrame(frame_ptr_t& frame);

        void finish();
    };
}
