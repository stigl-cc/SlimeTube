#include <models/video/VideoTypes.h>

extern "C" {
#include <libavutil/frame.h>
}
#include <stdexcept>


void SlimyTube::ResampleContextDeleter::operator()(SwrContext* p) const {
    if(p)
        swr_free(&p);
}


void SlimyTube::ResampleOutputDeleter::operator()(uint8_t* p) const {
    if(p)
        av_free(p);
}

void SlimyTube::FilterGraphDeleter::operator()(AVFilterGraph* p) const {
    if(p)
        avfilter_graph_free(&p);
}

void SlimyTube::FilterContextDeleter::operator()(AVFilterContext* p) const {
    if(p) {
    }
}

void SlimyTube::ResizeContextDeleter::operator()(SwsContext* p) const {
    if(p)
        sws_freeContext(p);
}

SlimyTube::video_proc_error::video_proc_error(bool isUserCaused, const char* const message) throw()
    : std::runtime_error(message),
      isUserCaused(isUserCaused) {
}


void SlimyTube::StreamDeleter::operator()(const AVStream* p) const {
    if(p) {
    }
}

void SlimyTube::FormatContextDeleter::operator()(AVFormatContext* p) const {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
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
