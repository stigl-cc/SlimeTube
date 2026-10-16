#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <models/images/ImageConverter.h>

#include <models/video/VideoTypes.h>
#include <models/Settings.hh>

#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
}

void SlimyTube::ImageConverter::encode(const std::filesystem::path& outputPath) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if(!codec)
        throw image_proc_error(false, "find encoder");

    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUVJ420P;
    const Settings& settings = Settings::getInstance();

    codec_cxt_ptr_t codec_cxt(avcodec_alloc_context3(codec));
    codec_cxt->width = settings.thumbnail_res.w;
    codec_cxt->height = settings.thumbnail_res.h;
    codec_cxt->time_base = {1, 1};
    codec_cxt->pix_fmt = pix_fmt;

    if(avcodec_open2(codec_cxt.get(), codec, nullptr) < 0)
        throw image_proc_error(false, "open codec");

    AVFormatContext* fmt_cxt = nullptr;
    avformat_alloc_output_context2(&fmt_cxt, nullptr, "mjpeg", outputPath.c_str());

    format_cxt_ptr_t format_cxt(fmt_cxt);
    if(format_cxt)
        throw image_proc_error(false, "alloc format context");

    stream_ptr_t stream(avformat_new_stream(format_cxt.get(), codec));
    avcodec_parameters_from_context(stream->codecpar, codec_cxt.get());

    if(format_cxt->oformat->flags & AVFMT_GLOBALHEADER)
        format_cxt->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(avformat_write_header(format_cxt.get(), nullptr) < 0)
        throw image_proc_error(false, "write header");

    resolution_t res_src = {
        .w = mFrame->width,
        .h = mFrame->height
    };

    if(res_src.w <= 0 || res_src.h <= 0)
        throw image_proc_error(true, "bad image resolution");

    const resolution_t& res_dst
        = settings.thumbnail_res;

    int scaled_height = res_src.w * res_dst.h / res_dst.w;

    int yStart = scaled_height / 2;
    int yEnd = yStart + scaled_height;

    resize_cxt_ptr_t resize_ctx(sws_getContext(res_src.w, scaled_height, static_cast<AVPixelFormat>(mFrame->format), res_dst.w, res_dst.h, pix_fmt, SWS_POINT, nullptr, nullptr, nullptr));
    if(!resize_ctx)
        throw image_proc_error(false, "alloc sws context");

    unique_frame_ptr_t scaled_frame(av_frame_alloc());
    if(!scaled_frame)
        throw image_proc_error(false, "alloc frame");

    scaled_frame->format = pix_fmt;
    scaled_frame->width = res_dst.w;
    scaled_frame->height = res_dst.h;
    av_frame_get_buffer(scaled_frame.get(), 32);

    sws_scale(resize_ctx.get(), )
}

SlimyTube::ImageConverter::ImageConverter() {

}
