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
        resample_cxt_ptr_t mResampleContext;
        unique_frame_ptr_t mResampledFrame;

        filter_graph_ptr_t mFilterGraph;
        filter_cxt_ptr_t mFilterContextSource;
        filter_cxt_ptr_t mFilterContextSink;

        AVCodecParameters mAudioCodecParams;
        AVCodecParameters mVideoCodecParams;

        void setupResample(int srcRate, int srcSamples, const AVChannelLayout* srcLayout, AVSampleFormat sampFormat);
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
