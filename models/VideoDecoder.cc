#include <cerrno>
#include <models/VideoTranscoder.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

#include <optional>
#include <stdexcept>
#include <string>

SlimyTube::VideoDecoder::error::error(const char* const message) throw() : std::runtime_error(message) { }

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

    if(mVideoStreamIndex >= 0)
        setupCodec(mFormatContext->streams[mVideoStreamIndex], mVideoCodec, mVideoCodecContext);

    if(mAudioStreamIndex >= 0)
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

bool SlimyTube::VideoDecoder::decodeFrame() {
    int ret;

    ret = avcodec_send_packet(mVideoCodecContext.get(), mPacket.get());
    if(ret < 0 && ret != AVERROR(EAGAIN))
        throw error("sending packet to decoder");

    ret = avcodec_receive_frame(mVideoCodecContext.get(), mFrame.get());
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        av_packet_unref(mPacket.get()); // TODO: Figure out where it actually belongs
        return false;
    } else if(ret < 0)
        throw error("receiving frame from decoder");

    return true;
}

std::optional<std::weak_ptr<AVFrame>> SlimyTube::VideoDecoder::getFrame() {
    int ret;

    ret = av_read_frame(mFormatContext.get(), mPacket.get());
    if(ret < 0)
        return std::nullopt;

    if(mPacket->stream_index == mVideoStreamIndex) {
        if(!decodeFrame())
            return std::nullopt;
    } else if(mPacket->stream_index != mAudioStreamIndex)
        printf("%d != %d\n", mPacket->stream_index, mAudioStreamIndex);
    return mFrame;
}

void SlimyTube::VideoDecoder::getFrames(std::function<void(weak_frame_ptr_t)> callback) {
    int ret;
    while(av_read_frame(mFormatContext.get(), mPacket.get()) >= 0) {
        if(mPacket->stream_index != mVideoStreamIndex)
            continue;

        ret = avcodec_send_packet(mVideoCodecContext.get(), mPacket.get());
        if(ret < 0)
            throw error("sending packet to decoder");

        while(ret >= 0) {
            ret = avcodec_receive_frame(mVideoCodecContext.get(), mFrame.get());
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if(ret < 0)
                throw error("receiving frame from decoder");

            callback(mFrame);
        }
        av_packet_unref(mPacket.get());
    }
}
