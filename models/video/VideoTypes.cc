#include <models/video/VideoTypes.h>

extern "C" {
#include <libavutil/frame.h>
}
#include <stdexcept>

SlimyTube::video_proc_error::video_proc_error(bool isUserCaused, const char* const message) throw()
    : std::runtime_error(message),
      isUserCaused(isUserCaused) {
}

void SlimyTube::FrameDeleter::operator()(AVFrame* p) const {
    if(p)
        av_free(p);
}

void SlimyTube::CodecContextDeleter::operator()(AVCodecContext* p) const {
    if(p)
        avcodec_free_context(&p);
}

void SlimyTube::PacketDeleter::operator()(AVPacket* p) const {
    if(p)
        av_packet_free(&p);
}
