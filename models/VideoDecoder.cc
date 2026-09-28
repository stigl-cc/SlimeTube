#include <libavutil/frame.h>
#include <models/VideoTranscoder.h>

#include <libavcodec/packet.h>
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <optional>
#include <stdexcept>
#include <string>

SlimyTube::VideoDecoder::error::error(const char* const message) throw() : std::runtime_error(message) { }

void SlimyTube::VideoDecoder::CodecContextDeleter::operator()(AVCodecContext* p) const {
    if(p)
        avcodec_free_context(&p);
}

void SlimyTube::VideoDecoder::PacketDeleter::operator()(AVPacket* p) const {
    if(p)
        av_packet_free(&p);
}

void SlimyTube::VideoDecoder::FormatContextDeleter::operator()(AVFormatContext* p) const {
    if(p)
        avformat_close_input(&p);
}

void SlimyTube::VideoDecoder::readBlob(const std::string& path) {
    AVFormatContext* fmt_cxt = nullptr;
    int ret;

    fmt_cxt = avformat_alloc_context();
    if(!fmt_cxt)
        throw error("allocate AVFormatContext");

    ret = avformat_open_input(&fmt_cxt, path.c_str(), nullptr, nullptr);
    if(ret < 0)
        throw error("open input blob");
    mFormatContext.reset(fmt_cxt);

    ret = avformat_find_stream_info(fmt_cxt, nullptr);
    if(ret < 0)
        throw error("retrieve InputStream info");
}

void SlimyTube::VideoDecoder::setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext) {
    if(!stream)
        throw error("Stream it not valid");

    codec.reset(avcodec_find_decoder(stream->codecpar->codec_id));
    if(!codec.get())
        throw error("find codec");

    codecContext.reset(avcodec_alloc_context3(codec.get()));
    if(!codecContext.get())
        throw error("allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), stream->codecpar) < 0)
        throw error("fill codec context");

    if(avcodec_open2(codecContext.get(), codec.get(), nullptr) < 0)
        throw error("open codec");
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

SlimyTube::VideoDecoder::VideoDecoder(const std::string& path) {
    readBlob(path);
    selectStreams();

    setupCodec(mFormatContext->streams[mVideoStreamIndex], mVideoCodec, mVideoCodecContext);
    setupCodec(mFormatContext->streams[mAudioStreamIndex], mAudioCodec, mAudioCodecContext);

    mPacket.reset(av_packet_alloc());
    if(!mPacket.get()) {
        throw error("allocate AVPacket");
    }

    mFrame.reset(av_frame_alloc(), [](AVFrame* p) -> void {
        av_frame_free(&p);
    });

    if(!mFrame.get()) {
        throw error("allocate AVFrame");
    }
}

std::optional<std::weak_ptr<AVFrame>> SlimyTube::VideoDecoder::getFrame() {
    if(av_read_frame(mFormatContext.get(), mPacket.get()) < 0) {
        return std::nullopt;
    }

    avcodec_send_packet(mVideoCodecContext.get(), mPacket.get());
    avcodec_receive_frame(mVideoCodecContext.get(), mFrame.get());
    return mFrame;
}
