#include <models/video/VideoEncoder.h>
#include <format>
#include <libavfilter/avfilter.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <models/video/VideoTypes.h>
#include <models/video/VideoTranscoder.h>
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

    avcodec_parameters_from_context(mVideoStream->codecpar, mVideoCodecContext.get());
    avcodec_parameters_from_context(mAudioStream->codecpar, mAudioCodecContext.get());

    if(avformat_write_header(mFormatContext.get(), &opts) < 0)
        throw video_proc_error(false, "write header");
}

void SlimyTube::VideoEncoder::setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext) {
    codec = avcodec_find_encoder(params.codec_id);
    if(!codec)
        throw video_proc_error(false, "find codec");

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

void SlimyTube::VideoEncoder::setupVideoFilter(AVRational targetFps) {
    int ret;
    mVideoFilterGraph.reset(avfilter_graph_alloc());
    if(!mVideoFilterGraph)
        throw video_proc_error(false, "allocate filter graph");

    const AVFilter* filter;
    AVFilterContext *filter_context = nullptr;

    const std::string in_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect=1/1",
        mTarget.width, mTarget.height,
        av_get_pix_fmt_name(static_cast<AVPixelFormat>(mVideoCodecParams.format)),
        mVideoSourceTimeBase.num, mVideoSourceTimeBase.den
    );

    const std::string args = std::format(
        "fps={}/{},scale={}:{}",
        targetFps.num, targetFps.den,
        mTarget.width, mTarget.height
    );

    // source filter
    filter = avfilter_get_by_name("buffer");
    ret = avfilter_graph_create_filter(&filter_context, filter, "in", in_args.c_str(), nullptr, mVideoFilterGraph.get());
    mVideoFilterContextSource.reset(filter_context);
    if(!mVideoFilterContextSource) throw video_proc_error(false, "create filter");

    // sink filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&filter_context, filter, "out", nullptr, nullptr, mVideoFilterGraph.get());
    mVideoFilterContextSink.reset(filter_context);
    if(!mVideoFilterContextSink) throw video_proc_error(false, "create filter");

    // filer processing part
    AVFilterInOut
        *in = avfilter_inout_alloc(),
        *out = avfilter_inout_alloc();

    in->name = av_strdup("in");
    in->filter_ctx = mVideoFilterContextSource.get();
    in->pad_idx = 0;

    out->name = av_strdup("out");
    out->filter_ctx = mVideoFilterContextSink.get();
    out->pad_idx = 0;

    ret = avfilter_graph_parse_ptr(mVideoFilterGraph.get(), args.c_str(), &out, &in, nullptr);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    if(ret < 0)
        throw video_proc_error(false, "graph parse str");

    ret = avfilter_graph_config(mVideoFilterGraph.get(), nullptr);
    if(ret < 0)
        throw video_proc_error(false, "configure filter graph");
}

void SlimyTube::VideoEncoder::setupAudioFilter(int srcRate, AVSampleFormat srcFormat, const AVChannelLayout& srcLayout) {
    int ret;
    mAudioFilterGraph.reset(avfilter_graph_alloc());
    if(!mAudioFilterGraph)
        throw video_proc_error(false, "allocate filter graph");

    const AVFilter* filter;
    AVFilterContext *filter_context = nullptr;

    char layout_buffer[128];
    av_channel_layout_describe(&srcLayout, layout_buffer, sizeof(layout_buffer));

    const std::string in_args = std::format(
        "sample_rate={}:sample_fmt={}:channel_layout={}",
        srcRate, av_get_sample_fmt_name(srcFormat), layout_buffer
    );

    av_channel_layout_describe(&mAudioCodecParams.ch_layout, layout_buffer, sizeof(layout_buffer));
    const std::string args = std::format(
        "aresample={},aformat=sample_fmts={}:channel_layouts={}",
        mAudioCodecParams.sample_rate,
        av_get_sample_fmt_name(static_cast<AVSampleFormat>(mAudioCodecParams.format)),
        layout_buffer
    );

    // source filter
    filter = avfilter_get_by_name("abuffer");
    ret = avfilter_graph_create_filter(&filter_context, filter, "in", in_args.c_str(), nullptr, mAudioFilterGraph.get());
    mAudioFilterContextSource.reset(filter_context);
    if(!mAudioFilterContextSource) throw video_proc_error(false, "create filter");

    // sink filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("abuffersink");
    ret = avfilter_graph_create_filter(&filter_context, filter, "out", nullptr, nullptr, mAudioFilterGraph.get());
    mAudioFilterContextSink.reset(filter_context);
    if(!mAudioFilterContextSink) throw video_proc_error(false, "create filter");

    // filer processing part
    AVFilterInOut
        *in = avfilter_inout_alloc(),
        *out = avfilter_inout_alloc();

    in->name = av_strdup("in");
    in->filter_ctx = mAudioFilterContextSource.get();
    in->pad_idx = 0;

    out->name = av_strdup("out");
    out->filter_ctx = mAudioFilterContextSink.get();
    out->pad_idx = 0;

    ret = avfilter_graph_parse_ptr(mAudioFilterGraph.get(), args.c_str(), &out, &in, nullptr);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    if(ret < 0)
        throw video_proc_error(false, "graph parse str");

    ret = avfilter_graph_config(mAudioFilterGraph.get(), nullptr);
    if(ret < 0)
        throw video_proc_error(false, "configure filter graph");
}

SlimyTube::VideoEncoder::VideoEncoder(const std::string& path, const VideoTarget& target,
    AVRational videoTimeBase,
    AVRational audioTimeBase)
    : mAudioSourceTimeBase(audioTimeBase),
      mVideoSourceTimeBase(videoTimeBase),
      mTarget(target) {
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

    mAudioSourceTimeBase = std::exchange(other.mAudioSourceTimeBase, {});
    mVideoSourceTimeBase = std::exchange(other.mVideoSourceTimeBase, {});

    mAudioCodecParams = std::exchange(other.mAudioCodecParams, {});
    mVideoCodecParams = std::exchange(other.mVideoCodecParams, {});

    mVideoFilterGraph = std::move(other.mVideoFilterGraph);
    mVideoFilterContextSource = std::move(other.mVideoFilterContextSource);
    mVideoFilterContextSink = std::move(other.mVideoFilterContextSink);

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

    mAudioSourceTimeBase = std::exchange(other.mAudioSourceTimeBase, {});
    mVideoSourceTimeBase = std::exchange(other.mVideoSourceTimeBase, {});

    mAudioCodecParams = std::exchange(other.mAudioCodecParams, {});
    mVideoCodecParams = std::exchange(other.mVideoCodecParams, {});

    mVideoFilterGraph = std::move(other.mVideoFilterGraph);
    mVideoFilterContextSource = std::move(other.mVideoFilterContextSource);
    mVideoFilterContextSink = std::move(other.mVideoFilterContextSink);

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
    if(frame)
        av_frame_unref(frame);
    av_packet_unref(output_packet);
}

void SlimyTube::VideoEncoder::putVideoFrame(const frame_ptr_t& frame) {
    if(frame) frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw video_proc_error(false, "allocate output packet");

    int ret;
    if(frame) {
        if(!mVideoFilterGraph)
            setupVideoFilter({Settings::getInstance().encoder_frame_rate, 1});

        ret = av_buffersrc_add_frame_flags(mVideoFilterContextSource.get(), frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "push frame to filter");
    } else {
        ret = av_buffersrc_add_frame_flags(mVideoFilterContextSource.get(), nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "push frame to filter");
    }

    AVFrame* filtered_frame = av_frame_alloc();
    if(!filtered_frame)
        throw video_proc_error(false, "alloc (filter) frame");

    while(true) {
        int ret = av_buffersink_get_frame(mVideoFilterContextSink.get(), filtered_frame);
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
    while (ret >= 0) {
        ret = avcodec_receive_packet(mAudioCodecContext.get(), output_packet);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            throw video_proc_error(false, "receive packet from encoder");

        output_packet->stream_index = 1;
        av_packet_rescale_ts(output_packet, mAudioCodecContext->time_base, mAudioStream->time_base);
        if(av_interleaved_write_frame(mFormatContext.get(), output_packet) < 0)
            throw video_proc_error(false, "writing packet to encoder");
    }

    av_frame_unref(frame);
    av_packet_unref(output_packet);
}

void SlimyTube::VideoEncoder::putAudioFrame(const frame_ptr_t& frame) {
    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw video_proc_error(false, "allocate output packet");

    int ret;
    if(frame) {
        if(!mAudioFilterGraph)
            setupAudioFilter(frame->sample_rate, static_cast<AVSampleFormat>(frame->format), frame->ch_layout );

        ret = av_buffersrc_add_frame_flags(mAudioFilterContextSource.get(), frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "push frame to filter");
    } else {
        ret = av_buffersrc_add_frame_flags(mAudioFilterContextSource.get(), nullptr, AV_BUFFERSRC_FLAG_KEEP_REF);
        if(ret < 0)
            throw video_proc_error(false, "push frame to filter");
    }

    AVFrame* filtered_frame = av_frame_alloc();
    if(!filtered_frame)
        throw video_proc_error(false, "alloc (filter) frame");

    while(true) {
        int ret = av_buffersink_get_frame(mAudioFilterContextSink.get(), filtered_frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if(ret < 0) {
            av_frame_free(&filtered_frame);
            throw video_proc_error(false, "pull frame from filter");
        }

        putAudioPacket(output_packet, filtered_frame);
    }

    if(!frame)
        putAudioPacket(output_packet, nullptr);

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
