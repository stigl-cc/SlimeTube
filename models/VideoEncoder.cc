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

    videoStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(videoStream->codecpar, &videoParams);

    audioStream.reset(avformat_new_stream(mFormatContext.get(), nullptr));
    avcodec_parameters_copy(audioStream->codecpar, &audioParams);
}

void SlimyTube::VideoEncoder::setupCodec(AVCodecParameters params, const AVCodec*& codec, codec_cxt_ptr_t& codecContext) {
    codec = avcodec_find_encoder(params.codec_id);
    codecContext.reset(avcodec_alloc_context3(codec));
    if(!codecContext.get())
        throw error("allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), &params) < 0)
        throw error("fill codec context");

    if(params.codec_type == AVMEDIA_TYPE_VIDEO) {
        codecContext->time_base = { codecContext->framerate.den, codecContext->framerate.num };
    } else if(params.codec_type == AVMEDIA_TYPE_AUDIO) {
        codecContext->time_base = {1, codecContext->sample_rate};
    }

    if(avcodec_open2(codecContext.get(), codec, nullptr) < 0)
        throw error("open codec");
}

SlimyTube::VideoEncoder::VideoEncoder(const std::string& path, int width, int height, int rate, AVCodecID videoCodec, AVCodecID audioCodec) {
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
