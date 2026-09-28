#include <models/VideoEncoder.hh>

#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdexcept>
#include <string>



void SlimyTube::VideoDecoder::CodecDeleter::operator()(const AVCodec* p) {
    // TODO
}

void SlimyTube::VideoDecoder::CodecContextDeleter ::operator()(AVCodecContext* p) {
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

void SlimyTube::VideoDecoder::setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext) {
    if(!stream)
        throw std::runtime_error("Stream it not valid");

    codec.reset(avcodec_find_decoder(stream->codecpar->codec_id));
    if(!codec.get())
        throw std::runtime_error("Failed to find the codec");

    codecContext.reset(avcodec_alloc_context3(codec.get()));
    if(!codecContext.get())
        throw std::runtime_error("Failed to allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), stream->codecpar) < 0)
        throw std::runtime_error("Failed to fill codec context");

    if(avcodec_open2(codecContext.get(), codec.get(), nullptr) < 0)
        throw std::runtime_error("Failed to open codec");
}

void SlimyTube::VideoDecoder::selectStreams() {
    // Select first or default streams
    for(int i =0; i < inputFormatContext->nb_streams; i++) {
        const AVMediaType& media_type = inputFormatContext->streams[i]->codecpar->codec_type;
        bool is_default = inputFormatContext->streams[i]->disposition & AV_DISPOSITION_DEFAULT;
        if(media_type == AVMEDIA_TYPE_VIDEO) {
            if(videoStreamIndex == -1 || is_default) {
                videoStreamIndex = i;
            }
        }
        else if(media_type == AVMEDIA_TYPE_AUDIO) {
            if(audioStreamIndex == -1 || is_default) {
                audioStreamIndex = i;
            }
        }
    }
}

void SlimyTube::VideoDecoder::decode() {
    setupCodec(inputFormatContext->streams[videoStreamIndex], videoCodec, videoCodecContext);
    setupCodec(inputFormatContext->streams[audioStreamIndex], audioCodec, audioCodecContext);
}

void SlimyTube::VideoEncoder::OutputFormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
}
