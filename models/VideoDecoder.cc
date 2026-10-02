#include <models/VideoTranscoder.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
#include <libavcodec/codec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
}

#include <stdexcept>
#include <string>
#include <cerrno>

SlimyTube::VideoDecoder::error::error(const char* const message) throw() : std::runtime_error(message) { }
SlimyTube::VideoDecoder::user_error::user_error(const char* const message) throw() : std::runtime_error(message) { }

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

void SlimyTube::VideoDecoder::setupCodec(const AVStream* stream, const AVCodec*& codec, codec_cxt_ptr_t& codecContext) {
    if(!stream)
        throw user_error("stream not valid");

    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!codec)
        throw user_error("find codec");

    codecContext.reset(avcodec_alloc_context3(codec));
    if(!codecContext.get())
        throw error("allocate memory for codec context");

    if(avcodec_parameters_to_context(codecContext.get(), stream->codecpar) < 0)
        throw error("fill codec context");

    if(avcodec_open2(codecContext.get(), codec, nullptr) < 0)
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

void SlimyTube::VideoDecoder::getFrames(video_callback_t videoCallback, audio_callback_t audioCallback) {
    int ret;
    while(av_read_frame(mFormatContext.get(), mPacket.get()) >= 0) {
        if(mPacket->stream_index == mVideoStreamIndex) {
            mCurrentCodecUsed = &mVideoCodecContext;
        } else if(mPacket->stream_index == mAudioStreamIndex) {
            mCurrentCodecUsed = &mAudioCodecContext;
        } else
            continue;

        ret = avcodec_send_packet(mCurrentCodecUsed->get(), mPacket.get());
        if(ret < 0)
            throw error(std::string("sending packet to decoder" + std::to_string(ret)).c_str());

        while(ret >= 0) {
            ret = avcodec_receive_frame(mCurrentCodecUsed->get(), mFrame.get());
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if(ret < 0)
                throw error("receiving frame from decoder");

            if(mCurrentCodecUsed->get()->codec_type == AVMEDIA_TYPE_VIDEO)
                videoCallback(mFrame);
            else audioCallback(mFrame);
        }
        av_packet_unref(mPacket.get());
    }
}

void SlimyTube::VideoDecoder::getTimeBase(AVRational& video, AVRational& audio) {
    video = mFormatContext->streams[mVideoStreamIndex]->time_base;
    audio = mFormatContext->streams[mAudioStreamIndex]->time_base;
}
