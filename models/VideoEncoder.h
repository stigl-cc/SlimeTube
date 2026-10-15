#pragma once
#include <memory>
#include <models/VideoTypes.h>
extern "C" {
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/codec_par.h>
#include <libavfilter/avfilter.h>
}

namespace SlimyTube {
    class VideoEncoder {
        private:
        static constexpr const std::string_view
            format_name = "mp4";

        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        struct StreamDeleter { void operator()(const AVStream*) const; };
        typedef std::unique_ptr<AVStream, StreamDeleter> stream_ptr_t;

        struct ResizeContextDeleter { void operator()(SwsContext*) const; };
        typedef std::unique_ptr<SwsContext, ResizeContextDeleter> resize_cxt_ptr_t;

        struct ResampleContextDeleter { void operator()(SwrContext*) const; };
        typedef std::unique_ptr<SwrContext, ResampleContextDeleter> resample_cxt_ptr_t;

        struct ResampleOutputDeleter { void operator()(uint8_t*) const; };
        typedef std::unique_ptr<uint8_t, ResampleOutputDeleter> resample_out_ptr_t;

        struct FilterGraphDeleter { void operator()(AVFilterGraph*) const; };
        typedef std::unique_ptr<AVFilterGraph, FilterGraphDeleter> filter_graph_ptr_t;

        struct FilterContextDeleter { void operator()(AVFilterContext*) const; };
        typedef std::unique_ptr<AVFilterContext, FilterContextDeleter> filter_cxt_ptr_t;

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

        unique_frame_ptr_t mScaledFrame;
        resize_cxt_ptr_t mResizeContext;

        int mResampledSamplesEmitted;
        resample_cxt_ptr_t mResampleContext;
        unique_frame_ptr_t mResampledFrame;

        AVCodecParameters mAudioCodecParams;
        AVCodecParameters mVideoCodecParams;

        filter_graph_ptr_t mFilterGraph;
        filter_cxt_ptr_t mFilterContextSource;
        filter_cxt_ptr_t mFilterContextSink;
        filter_cxt_ptr_t mFilterContextFps;

        void setupResample(int srcRate, int srcSamples, const AVChannelLayout* srcLayout, AVSampleFormat sampFormat);
        void setupResize(int srcW, int srcH, AVPixelFormat fmt);
        void openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams);
        void setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);
        void setupFilter(AVRational targetFps);

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
