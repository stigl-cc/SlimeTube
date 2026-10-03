#include <models/VideoTranscoder.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_par.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
#include <libavcodec/defs.h>
#include <libavcodec/packet.h>
#include <libavutil/mathematics.h>
}

SlimyTube::VideoEncoder::error::error(const char* const message) throw() : std::runtime_error(message) { }

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

void SlimyTube::VideoEncoder::openOutput(const std::string& path, const AVCodecParameters& videoParams, const AVCodecParameters& audioParams) {
    AVFormatContext* fmt_cxt = nullptr;
    avformat_alloc_output_context2(&fmt_cxt, nullptr, format_name.data(), path.c_str());
    if(!fmt_cxt)
        throw error("open output");

    mFormatContext.reset(fmt_cxt);

    mVideoStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(mVideoStream->codecpar, &videoParams);
    mVideoStream->time_base = {1, 60 * 256};

    mAudioStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(mAudioStream->codecpar, &audioParams);

    if(mFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        mFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(!(mFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if(avio_open(&mFormatContext->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
            throw error("open output file");
        }
    }

    if(avformat_write_header(mFormatContext.get(), nullptr) < 0) {
        throw error("write hedaer");
    }
}

void SlimyTube::VideoEncoder::setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext) {
    codec = avcodec_find_encoder(params.codec_id);
    codecContext.reset(avcodec_alloc_context3(codec));
    if(!codecContext.get())
        throw error("allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), &params) < 0)
        throw error("fill codec context");

    if(params.codec_type == AVMEDIA_TYPE_VIDEO) {
        codecContext->time_base = { 1, 30 };
    } else if(params.codec_type == AVMEDIA_TYPE_AUDIO) {
        codecContext->time_base = {1, codecContext->sample_rate};
    }

    if(avcodec_open2(codecContext.get(), codec, nullptr) < 0)
        throw error("open codec");
}

SlimyTube::VideoEncoder::VideoEncoder(const std::string& path, int width, int height, int rate,
    AVRational videoTimeBase,
    AVRational audioTimeBase, AVCodecID videoCodec, AVCodecID audioCodec) : mAudioSourceTimeBase(audioTimeBase),
                                                                            mVideoSourceTimeBase(videoTimeBase) {

    AVCodecParameters video_params = {
        .codec_type = AVMEDIA_TYPE_VIDEO,
        .codec_id = videoCodec,
        .format = AVPixelFormat::AV_PIX_FMT_YUV420P,
        .profile = AV_PROFILE_H264_HIGH,
        .width = width,
        .height = height,
        .framerate = { 60, 1 }
    };
    setupCodec(video_params, mVideoCodec, mVideoCodecContext);

    AVCodecParameters audio_params = {
        .codec_type = AVMEDIA_TYPE_AUDIO,
        .codec_id = audioCodec,
        .format = AVSampleFormat::AV_SAMPLE_FMT_FLTP,
        .profile = AV_PROFILE_AAC_LOW,
        .sample_rate = 48000,
    };
    av_channel_layout_default(&audio_params.ch_layout, 2);
    setupCodec(audio_params, mAudioCodec, mAudioCodecContext);

    openOutput(path, video_params, audio_params);
}

void SlimyTube::VideoEncoder::putVideoFrame(frame_ptr_t& frame) {
    if(frame) frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw error("allocate output packet");

    int ret;
    ret = avcodec_send_frame(mVideoCodecContext.get(), frame.get());

    while (ret >= 0) {
        ret = avcodec_receive_packet(mVideoCodecContext.get(), output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw error("receive packet from encoder");
        }

        output_packet->stream_index = 0;
        output_packet->pts = av_rescale_q_rnd(output_packet->pts, mVideoSourceTimeBase, mVideoStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        output_packet->dts = av_rescale_q_rnd(output_packet->dts, mVideoSourceTimeBase, mVideoStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        output_packet->duration = av_rescale_q(output_packet->duration, mVideoSourceTimeBase, mVideoStream->time_base);;

        //av_packet_rescale_ts(output_packet, mVideoSourceTimeBase, mVideoStream->time_base);
        if(av_interleaved_write_frame(mFormatContext.get(), output_packet) < 0)
            throw error("writing packet to encoder");
    }

    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
}

void SlimyTube::VideoEncoder::putAudioFrame(frame_ptr_t& frame) {
    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw error("allocate output packet");

    int ret;
    ret = avcodec_send_frame(mAudioCodecContext.get(), frame.get());

    while (ret >= 0) {
        ret = avcodec_receive_packet(mAudioCodecContext.get(), output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw error("receive packet from encoder");
        }

        output_packet->stream_index = 1;
        av_packet_rescale_ts(output_packet, mAudioSourceTimeBase, mAudioStream->time_base);

        if(av_interleaved_write_frame(mFormatContext.get(), output_packet) < 0)
            throw error("writing packet to encoder");
    }

    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
}

void SlimyTube::VideoEncoder::write() {
    av_write_trailer(mFormatContext.get());
}

// 02.97
// 05.13
