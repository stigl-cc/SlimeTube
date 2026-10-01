#include <models/VideoTranscoder.h>

void SlimyTube::CodecContextDeleter::operator()(AVCodecContext* p) const {
    if(p)
        avcodec_free_context(&p);
}

void SlimyTube::PacketDeleter::operator()(AVPacket* p) const {
    if(p)
        av_packet_free(&p);
}

