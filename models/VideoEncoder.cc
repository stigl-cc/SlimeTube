#include "VideoEncoder.h"
#include "VideoTypes.h"
#include <format>
#include <libavfilter/avfilter.h>
#include <models/VideoTranscoder.h>
#include <utility>
#include <models/Settings.hh>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

#include <libavcodec/defs.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

void SlimyTube::VideoEncoder::StreamDeleter::operator()(const AVStream* p) const {
    if(p) {
    }
}

void SlimyTube::VideoEncoder::FormatContextDeleter::operator()(AVFormatContext* p) const {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
}

void SlimyTube::VideoEncoder::ResizeContextDeleter::operator()(SwsContext* p) const {
    if(p)
        sws_freeContext(p);
}


void SlimyTube::VideoEncoder::ResampleContextDeleter::operator()(SwrContext* p) const {
    if(p)
        swr_free(&p);
}


void SlimyTube::VideoEncoder::ResampleOutputDeleter::operator()(uint8_t* p) const {
    if(p)
        av_free(p);
}

void SlimyTube::VideoEncoder::FilterGraphDeleter::operator()(AVFilterGraph* p) const {
    if(p)
        avfilter_graph_free(&p);
}

void SlimyTube::VideoEncoder::FilterContextDeleter::operator()(AVFilterContext* p) const {
    if(p) {
    }
}

void SlimyTube::VideoEncoder::setupResample(int srcRate, int srcSamples, const AVChannelLayout* srcLayout, AVSampleFormat sampFormat) {
    SwrContext* cxt = swr_alloc();
    if(!cxt)
        throw video_proc_error(false, "allocat swr context");

    int ret;

    const AVSampleFormat output_format = static_cast<AVSampleFormat>(mAudioCodecParams.format);
    av_opt_set_chlayout(cxt, "in_chlayout", srcLayout, 0);
    av_opt_set_sample_fmt(cxt, "in_sample_fmt",  sampFormat, 0);
    av_opt_set_int(cxt, "in_sample_rate", srcRate, 0);

    av_opt_set_chlayout(cxt, "out_chlayout", &mAudioCodecParams.ch_layout, 0);
    av_opt_set_sample_fmt(cxt, "out_sample_fmt", output_format, 0);
    av_opt_set_int(cxt, "out_sample_rate",    mAudioCodecParams.sample_rate, 0);

    ret = swr_init(cxt);
    if(ret < 0) {
        swr_free(&cxt);
        throw video_proc_error(false, "init resampler");
    }

    mResampleContext.reset(cxt);

    int output_samples = swr_get_out_samples(cxt, srcSamples);
    mResampledFrame.reset(av_frame_alloc());

    mResampledFrame->format = output_format;
    mResampledFrame->sample_rate = mAudioCodecParams.sample_rate;
    mResampledFrame->nb_samples = output_samples;
    av_channel_layout_copy(&mResampledFrame->ch_layout, &mAudioCodecParams.ch_layout);

    ret = av_frame_get_buffer(mResampledFrame.get(), 0);

    if(ret < 0)
        throw video_proc_error(false, "alloc samples");
}

void SlimyTube::VideoEncoder::setupResize(int srcW, int srcH, AVPixelFormat fmt) {
    SwsContext* cxt = sws_getContext(srcW, srcH, fmt, mTarget.width, mTarget.height, static_cast<AVPixelFormat>(mVideoCodecParams.format), SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(!cxt)
        throw video_proc_error(false, "alloc sws context");
    mResizeContext.reset(cxt);

    mScaledFrame.reset(av_frame_alloc());
    if(!mScaledFrame)
        throw video_proc_error(false, "alloc frame");

    mScaledFrame->format = AV_PIX_FMT_YUV420P;
    mScaledFrame->width = mTarget.width;
    mScaledFrame->height = mTarget.height;
    av_frame_get_buffer(mScaledFrame.get(), 32);
}

void SlimyTube::VideoEncoder::openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams) {
    AVFormatContext* fmt_cxt = nullptr;
    avformat_alloc_output_context2(&fmt_cxt, nullptr, format_name.data(), path.c_str());

    if(!fmt_cxt)
        throw video_proc_error(false, "open output");

    mFormatContext.reset(fmt_cxt);

    mVideoStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(mVideoStream->codecpar, &videoParams);
    mVideoStream->time_base = mVideoCodecContext->time_base;

    mAudioStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(mAudioStream->codecpar, &audioParams);
    mAudioStream->time_base = mAudioCodecContext->time_base;

    if(mFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        mFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(!(mFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if(avio_open(&mFormatContext->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
            throw video_proc_error(false, "open output file");
        }
    }

    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "movflags", "+faststart", 0);

    if(avformat_write_header(mFormatContext.get(), &opts) < 0)
        throw video_proc_error(false, "write header");
}

void SlimyTube::VideoEncoder::setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext) {
    codec = avcodec_find_encoder(params.codec_id);
    codecContext.reset(avcodec_alloc_context3(codec));
    if(!codecContext.get())
        throw video_proc_error(false, "allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), &params) < 0)
        throw video_proc_error(false, "fill codec context");

    if(params.codec_type == AVMEDIA_TYPE_VIDEO) {
        codecContext->time_base = { 1, params.framerate.num };
    } else if(params.codec_type == AVMEDIA_TYPE_AUDIO) {
        codecContext->time_base = {1, codecContext->sample_rate};
    }

    if(avcodec_open2(codecContext.get(), codec, nullptr) < 0)
        throw video_proc_error(false, "open codec");
}

void SlimyTube::VideoEncoder::setupFilter(AVRational targetFps) {
    mFilterGraph.reset(avfilter_graph_alloc());
    if(!mFilterGraph)
        throw video_proc_error(false, "allocate filter graph");

    int ret;

    const AVFilter* filter;
    AVFilterContext *filter_context = nullptr;

    const std::string in_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect=1/1",
        mTarget.width, mTarget.height,
        static_cast<int32_t>(mVideoCodecParams.format),
        mVideoSourceTimeBase.num, mVideoSourceTimeBase.den
    );

    // Allocate and setup the source filter
    filter = avfilter_get_by_name("buffer");
    ret = avfilter_graph_create_filter(&filter_context, filter, "in", in_args.c_str(), nullptr, mFilterGraph.get());
    mFilterContextSource.reset(filter_context);
    if(!mFilterContextSource) throw video_proc_error(false, "create filter");

    // sink filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&filter_context, filter, "out", nullptr, nullptr, mFilterGraph.get());
    mFilterContextSink.reset(filter_context);
    if(!mFilterContextSink) throw video_proc_error(false, "create filter");

    // fps filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("fps");
    ret = avfilter_graph_create_filter(&filter_context, filter, "fps", std::format("{}/{}", targetFps.num, targetFps.den).c_str(), nullptr, mFilterGraph.get());
    mFilterContextFps.reset(filter_context);
    if(!mFilterContextFps) throw video_proc_error(false, "create filter");

    avfilter_link(mFilterContextSource.get(), 0, mFilterContextFps.get(), 0);
    avfilter_link(mFilterContextFps.get(), 0, mFilterContextSink.get(), 0);

    ret = avfilter_graph_config(mFilterGraph.get(), nullptr);
    if(ret < 0)
        throw video_proc_error(false, "configure filter graph");
}

SlimyTube::VideoEncoder::VideoEncoder(const std::string& path, const VideoTarget& target,
    AVRational videoTimeBase,
    AVRational audioTimeBase)
    : mAudioSourceTimeBase(audioTimeBase),
      mVideoSourceTimeBase(videoTimeBase),
      mTarget(target),
      mResampledSamplesEmitted(0) {
    mVideoCodecParams = {
        .codec_type = AVMEDIA_TYPE_VIDEO,
        .codec_id = target.videoCodec,
        .format = AVPixelFormat::AV_PIX_FMT_YUV420P,
        .bit_rate = target.v_bitrate,
        .profile = AV_PROFILE_H264_HIGH,
        .width = target.width,
        .height = target.height,
        .framerate = { Settings::getInstance().encoder_frame_rate, 1 },
    };
    setupCodec(mVideoCodecParams, mVideoCodec, mVideoCodecContext);

    mAudioCodecParams = {
        .codec_type = AVMEDIA_TYPE_AUDIO,
        .codec_id = target.audioCodec,
        .format = AVSampleFormat::AV_SAMPLE_FMT_FLTP,
        .bit_rate = target.a_bitrate,
        .profile = AV_PROFILE_AAC_LOW,
        .sample_rate = target.a_samprate,
    };

    av_channel_layout_default(&mAudioCodecParams.ch_layout, 2);
    setupCodec(mAudioCodecParams, mAudioCodec, mAudioCodecContext);

    openOutput(path, mVideoCodecParams, mAudioCodecParams);
}

SlimyTube::VideoEncoder::VideoEncoder(VideoEncoder&& other) {
    mFormatContext = std::move(other.mFormatContext);

    mAudioCodec = std::exchange(other.mAudioCodec, nullptr);
    mVideoCodec = std::exchange(other.mVideoCodec, nullptr);

    mAudioCodecContext = std::move(other.mAudioCodecContext);
    mVideoCodecContext = std::move(other.mVideoCodecContext);

    mVideoStream = std::move(other.mVideoStream);
    mAudioStream = std::move(other.mAudioStream);

    mAudioSourceTimeBase = other.mAudioSourceTimeBase;
    mVideoSourceTimeBase = other.mVideoSourceTimeBase;

    mScaledFrame = std::move(other.mScaledFrame);
    mResizeContext = std::move(other.mResizeContext);

    mResampleContext = std::move(other.mResampleContext);
    mResampledFrame = std::move(other.mResampledFrame);

    mAudioCodecParams = other.mAudioCodecParams;
    mVideoCodecParams = other.mVideoCodecParams;

    mTarget = std::move(other.mTarget);
}

SlimyTube::VideoEncoder& SlimyTube::VideoEncoder::operator=(VideoEncoder&& other) {
    if(&other == this)
        return *this;

    mFormatContext = std::move(other.mFormatContext);

    mAudioCodec = std::exchange(other.mAudioCodec, nullptr);
    mVideoCodec = std::exchange(other.mVideoCodec, nullptr);

    mAudioCodecContext = std::move(other.mAudioCodecContext);
    mVideoCodecContext = std::move(other.mVideoCodecContext);

    mVideoStream = std::move(other.mVideoStream);
    mAudioStream = std::move(other.mAudioStream);

    mAudioSourceTimeBase = other.mAudioSourceTimeBase;
    mVideoSourceTimeBase = other.mVideoSourceTimeBase;

    mScaledFrame = std::move(other.mScaledFrame);
    mResizeContext = std::move(other.mResizeContext);

    mResampleContext = std::move(other.mResampleContext);
    mResampledFrame = std::move(other.mResampledFrame);

    mTarget = std::move(other.mTarget);

    return *this;
}

void SlimyTube::VideoEncoder::putVideoPacket(AVPacket* output_packet, AVFrame* frame) {
    int ret;
    ret = avcodec_send_frame(mVideoCodecContext.get(), frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(mVideoCodecContext.get(), output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw video_proc_error(false, "receive packet from encoder");
        }

        output_packet->stream_index = 0;
        av_packet_rescale_ts(output_packet, mVideoCodecContext->time_base, mVideoStream->time_base);
        if(av_interleaved_write_frame(mFormatContext.get(), output_packet) < 0)
            throw video_proc_error(false, "writing packet to encoder");
    }
    av_frame_unref(frame);
    av_packet_unref(output_packet);
}

void SlimyTube::VideoEncoder::putVideoFrame(frame_ptr_t& frame) {
    if(frame) frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw video_proc_error(false, "allocate output packet");

    int ret;
    if(frame) {
        if(!mFilterGraph)
            setupFilter({Settings::getInstance().encoder_frame_rate, 1});
        if(!mResizeContext)
            setupResize(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format));

        sws_scale(mResizeContext.get(), frame->data, frame->linesize, 0, frame->height, mScaledFrame->data, mScaledFrame->linesize);
        av_frame_copy_props(mScaledFrame.get(), frame.get());
        mScaledFrame->pts = av_rescale_q(frame->pts, mVideoSourceTimeBase, mVideoCodecContext->time_base);
        mScaledFrame->pkt_dts = av_rescale_q(frame->pkt_dts, mVideoSourceTimeBase, mVideoCodecContext->time_base);

        ret = av_buffersrc_add_frame_flags(mFilterContextSource.get(), mScaledFrame.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "frame frame to filter");
    } else {
        ret = av_buffersrc_add_frame_flags(mFilterContextSource.get(), nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "frame frame to filter");
    }

    AVFrame* filtered_frame = av_frame_alloc();
    if(!filtered_frame)
        throw video_proc_error(false, "alloc (filter) frame");

    while(true) {
        int ret = av_buffersink_get_frame(mFilterContextSink.get(), filtered_frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if(ret < 0) {
            av_frame_free(&filtered_frame);
            throw video_proc_error(false, "pull frame from filter");
        }

        putVideoPacket(output_packet, filtered_frame);
    }

    if(!frame)
        putVideoPacket(output_packet, nullptr);

    av_packet_free(&output_packet);
    av_frame_free(&filtered_frame);
}

void SlimyTube::VideoEncoder::putAudioPacket(AVPacket* output_packet, AVFrame* frame) {
    int ret;
    ret = avcodec_send_frame(mAudioCodecContext.get(), frame);
    if(ret < 0)
        throw video_proc_error(false, "send frame to encoder");
    while (ret >= 0) {
        ret = avcodec_receive_packet(mAudioCodecContext.get(), output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            throw video_proc_error(false, "receive packet from encoder");

        output_packet->stream_index = 1;
        av_packet_rescale_ts(output_packet, mAudioCodecContext->time_base, mAudioStream->time_base);
        if(av_interleaved_write_frame(mFormatContext.get(), output_packet) < 0)
            throw video_proc_error(false, "writing packet to encoder");
    }
}

void SlimyTube::VideoEncoder::putAudioFrame(frame_ptr_t& frame) {
    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw video_proc_error(false, "allocate output packet");

    int ret;
    if(frame) {
        if(!mResampleContext)
            setupResample(frame->sample_rate, frame->nb_samples, &frame->ch_layout, static_cast<AVSampleFormat>(frame->format));

        ret = swr_convert(mResampleContext.get(), mResampledFrame->data, mResampledFrame->nb_samples, const_cast<const uint8_t**>(frame->data), frame->nb_samples);
        if(ret < 0)
            throw video_proc_error(false, "resample audio");

        mResampledFrame->pts = av_rescale_q(mResampledSamplesEmitted, AVRational{1, mAudioCodecParams.sample_rate}, mAudioCodecContext->time_base);
        mResampledFrame->nb_samples = ret;
        mResampledSamplesEmitted += ret;

        putAudioPacket(output_packet, mResampledFrame.get());
    } else if(mResampleContext) {
        while(true) {
            ret = swr_convert(mResampleContext.get(), mResampledFrame->data, mResampledFrame->nb_samples, nullptr, 0);
            if(ret < 0)
                throw video_proc_error(false, "flush resampled audio");
            if(ret == 0) break;
            mResampledFrame->pts = av_rescale_q(mResampledSamplesEmitted, AVRational{1, mAudioCodecParams.sample_rate}, mAudioCodecContext->time_base);
            mResampledFrame->nb_samples = ret;
            mResampledSamplesEmitted += ret;

            putAudioPacket(output_packet, mResampledFrame.get());
            av_packet_unref(output_packet);
        }

        putAudioPacket(output_packet, frame.get());
    } else {
        putAudioPacket(output_packet, frame.get());
    }

    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
}

void SlimyTube::VideoEncoder::finish() {
    // Flush the stream
    frame_ptr_t null_frame = { nullptr };
    putVideoFrame(null_frame);
    putAudioFrame(null_frame);

    mVideoStream->codecpar->bit_rate = mVideoCodecContext->bit_rate;
    mAudioStream->codecpar->bit_rate = mAudioCodecContext->bit_rate;
    if(av_write_trailer(mFormatContext.get()))
        throw video_proc_error(false, "write trailer");
}

const SlimyTube::VideoTarget& SlimyTube::VideoEncoder::getTarget() {
    return mTarget;
}
