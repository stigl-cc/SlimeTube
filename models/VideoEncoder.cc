#include <models/VideoEncoder.hh>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdexcept>
#include <string>

void SlimyTube::VideoEncoder::InputFormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        avformat_close_input(&p);
    }
}

void SlimyTube::VideoEncoder::OutputFormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
}

SlimyTube::VideoEncoder::VideoEncoder() : inputFormatContext(nullptr), outputFormatContext(nullptr) {

}

void SlimyTube::VideoEncoder::readBlob(const std::string& path) {
    AVFormatContext* input_fmt_cxt = nullptr;
    int ret;

    input_fmt_cxt = avformat_alloc_context();
    if(!input_fmt_cxt)
        throw std::runtime_error("Failed to allocate memory for context");

    ret = avformat_open_input(&input_fmt_cxt, path.c_str(), nullptr, nullptr);
    if(ret < 0)
        throw std::runtime_error("Failed to open input blob.");
    inputFormatContext.reset(input_fmt_cxt);

    ret = avformat_find_stream_info(input_fmt_cxt, nullptr);
    if(ret < 0)
        throw std::runtime_error("Failed to retrieve InputStream info.");
}

void SlimyTube::VideoEncoder::setupCodec(const AVStream* stream) {
    const AVCodec* codec;
    AVCodecContext* codec_context;

    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!codec)
        throw std::runtime_error("Failed to find the codec");

    codec_context = avcodec_alloc_context3(codec);
    if(!codec_context)
        throw std::runtime_error("Failed to allocate memory for codec context");

    if(avcodec_parameters_to_context(codec_context, stream->codecpar) < 0)
        throw std::runtime_error("Failed to fill codec context");

    if(avcodec_open2(codec_context, codec, nullptr) < 0)
        throw std::runtime_error("Failed to open codec");
}

void SlimyTube::VideoEncoder::decode() {
    for(int i =0; i < inputFormatContext->nb_streams; i++) {
        const AVMediaType& media_type = inputFormatContext->streams[i]->codecpar->codec_type;
        if(media_type == AVMEDIA_TYPE_VIDEO) {
            setupCodec(inputFormatContext->streams[i]);
        } else if(media_type == AVMEDIA_TYPE_AUDIO) {

        }
    }
}
