#include <libavcodec/packet.h>
#include <models/VideoEncoder.hh>

#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdexcept>
#include <string>


void SlimyTube::VideoDecoder::CodecDeleter::operator()(const AVCodec* p) {
    // TODO
}

void SlimyTube::VideoDecoder::CodecContextDeleter::operator()(AVCodecContext* p) {
    // TODO
}

void SlimyTube::VideoDecoder::PacketDeleter::operator()(AVPacket* p) {
    // TODO
}

void SlimyTube::VideoDecoder::FormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        avformat_close_input(&p);
    }
}

SlimyTube::VideoDecoder::VideoDecoder() {

}

void SlimyTube::VideoDecoder::readBlob(const std::string& path) {
    AVFormatContext* fmt_cxt = nullptr;
    int ret;

    fmt_cxt = avformat_alloc_context();
    if(!fmt_cxt)
        throw std::runtime_error("allocate AVFormatContext");

    ret = avformat_open_input(&fmt_cxt, path.c_str(), nullptr, nullptr);
    if(ret < 0)
        throw std::runtime_error("open input blob");
    mFormatContext.reset(fmt_cxt);

    ret = avformat_find_stream_info(fmt_cxt, nullptr);
    if(ret < 0)
        throw std::runtime_error("retrieve InputStream info");
}

void SlimyTube::VideoDecoder::setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext) {
    if(!stream)
        throw std::runtime_error("Stream it not valid");

    codec.reset(avcodec_find_decoder(stream->codecpar->codec_id));
    if(!codec.get())
        throw std::runtime_error("find codec");

    codecContext.reset(avcodec_alloc_context3(codec.get()));
    if(!codecContext.get())
        throw std::runtime_error("allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), stream->codecpar) < 0)
        throw std::runtime_error("fill codec context");

    if(avcodec_open2(codecContext.get(), codec.get(), nullptr) < 0)
        throw std::runtime_error("open codec");
}

void SlimyTube::VideoDecoder::selectStreams() {
    // Select first or default streams
    for(int i =0; i < mFormatContext->nb_streams; i++) {
        const AVMediaType& media_type = mFormatContext->streams[i]->codecpar->codec_type;
        bool is_default = mFormatContext->streams[i]->disposition & AV_DISPOSITION_DEFAULT;
        if(media_type == AVMEDIA_TYPE_VIDEO) {
            if(mVideoStreamIndex == -1 || is_default) {
                mVideoStreamIndex = i;
            }
        }
        else if(media_type == AVMEDIA_TYPE_AUDIO) {
            if(mAudioStreamIndex == -1 || is_default) {
                mAudioStreamIndex = i;
            }
        }
    }
}

void SlimyTube::VideoDecoder::decode() {
    selectStreams();

    setupCodec(mFormatContext->streams[mVideoStreamIndex], mVideoCodec, mVideoCodecContext);
    setupCodec(mFormatContext->streams[mAudioStreamIndex], mAudioCodec, mAudioCodecContext);

    mPacket.reset(av_packet_alloc());
    if(!mPacket.get()) {
        throw std::runtime_error("allocate AVPacket");
    }

    mFrame.reset(av_frame_alloc());
    if(!mFrame.get()) {
        throw std::runtime_error("allocate AVFrame");
    }
}

void SlimyTube::VideoEncoder::OutputFormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
}
