#include <models/VideoTranscoder.h>

void SlimyTube::VideoEncoder::OutputFormatContextDeleter::operator()(AVFormatContext* p) {
    if(p) {
        if(!(p->oformat->flags & AVFMT_NOFILE))
            avio_closep(&p->pb);
        avformat_free_context(p);
    }
}
