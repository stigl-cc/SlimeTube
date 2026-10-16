#include <models/images/ImageConverter.h>
#include <models/video/VideoTypes.h>
#include <models/Settings.hh>
#include <format>

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
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

    filter_graph_ptr_t filter_graph(avfilter_graph_alloc());
    if(!filter_graph)
        throw image_proc_error(false, "alloc filter graph");

    int ret;

    filter_cxt_ptr_t
        cxt_in, cxt_scale, cxt_crop, cxt_out;

    const AVFilter* filter;
    AVFilterContext *filter_context = nullptr;

    const std::string in_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        res_src.w, res_src.h,
        static_cast<int32_t>(mFrame->format),
        0, 0,
        mFrame->sample_aspect_ratio.num, mFrame->sample_aspect_ratio.den
    );

    const std::string crop_args = std::format(
        "{}:{}:(in_w-{})/2:(in_h-{})/2",
        res_src.w, res_src.w * res_dst.w / res_dst.h,
        res_src.w, res_src.w * res_dst.w / res_dst.h
    );

    const std::string scale_args = std::format("{}:{}", res_dst.w, res_dst.h);

    // source filter
    filter = avfilter_get_by_name("buffer");
    ret = avfilter_graph_create_filter(&filter_context, filter, "in", in_args.c_str(), nullptr, filter_graph.get());
    cxt_in.reset(filter_context);
    if(!cxt_in) throw image_proc_error(false, "create filter");

    // sink filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&filter_context, filter, "out", nullptr, nullptr, filter_graph.get());
    cxt_out.reset(filter_context);
    if(!cxt_out) throw video_proc_error(false, "create filter");

    // crop filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("crop");
    ret = avfilter_graph_create_filter(&filter_context, filter, "crop", crop_args.c_str(), nullptr, filter_graph.get());
    cxt_crop.reset(filter_context);
    if(!cxt_crop) throw video_proc_error(false, "create filter");

    // scale filter
    filter_context = nullptr;
    filter = avfilter_get_by_name("scale");
    ret = avfilter_graph_create_filter(&filter_context, filter, "crop", scale_args.c_str(), nullptr, filter_graph.get());
    cxt_scale.reset(filter_context);
    if(!cxt_scale) throw video_proc_error(false, "create filter");

    avfilter_link(cxt_in.get(), 0, cxt_crop.get(), 0);
    avfilter_link(cxt_crop.get(), 0, cxt_scale.get(), 0);
    avfilter_link(cxt_scale.get(), 0, cxt_out.get(), 0);

    ret = avfilter_graph_config(filter_graph.get(), nullptr);
    if(ret < 0)
        throw video_proc_error(false, "configure filter graph");

    unique_frame_ptr_t scaled_frame(av_frame_alloc());
    if(!scaled_frame)
        throw image_proc_error(false, "alloc frame");

    scaled_frame->format = pix_fmt;
    scaled_frame->width = res_dst.w;
    scaled_frame->height = res_dst.h;
    av_frame_get_buffer(scaled_frame.get(), 32);
}

SlimyTube::ImageConverter::ImageConverter() {

}
