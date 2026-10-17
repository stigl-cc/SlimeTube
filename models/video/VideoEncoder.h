#pragma once
#include <memory>
#include <models/video/VideoTypes.h>
extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/codec_par.h>
#include <libavfilter/avfilter.h>
}

namespace SlimyTube {
    class VideoEncoder {
        private:
        static constexpr const std::string_view
            format_name = "mp4";

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

        int mResampledSamplesEmitted;

        filter_graph_ptr_t mVideoFilterGraph;
        filter_cxt_ptr_t mVideoFilterContextSource;
        filter_cxt_ptr_t mVideoFilterContextSink;

        filter_graph_ptr_t mAudioFilterGraph;
        filter_cxt_ptr_t mAudioFilterContextSource;
        filter_cxt_ptr_t mAudioFilterContextSink;

        AVCodecParameters mAudioCodecParams;
        AVCodecParameters mVideoCodecParams;

        void openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams);
        void setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);
        void setupVideoFilter(AVRational targetFps);
        void setupAudioFilter(int srcRate, AVSampleFormat srcFormat, const AVChannelLayout& srcLayout);

        VideoTarget mTarget;

        void putVideoPacket(AVPacket*, AVFrame* frame);
        void putAudioPacket(AVPacket*, AVFrame* frame);

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

        const VideoTarget& getTarget();

        void finish();
    };
}
