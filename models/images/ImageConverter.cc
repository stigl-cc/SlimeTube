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
#include <libavfilter/buffersink.h>
#include <libavutil/mem.h>
#include <libavfilter/buffersrc.h>
}

SlimyTube::image_proc_error::image_proc_error(bool isUserCaused, const char* const message) throw()
    : std::runtime_error(message),
      isUserCaused(isUserCaused) {
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
    if(!format_cxt)
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

    // Set up filters for cropping and scaling the thumbnail blob
    int ret;
    const AVFilter* filter;
    AVFilterContext* filter_context;

    const std::string in_args = std::format(
        "video_size={}x{}:pix_fmt={}:time_base=1/1:pixel_aspect={}/{}",
        res_src.w, res_src.h,
        static_cast<int32_t>(mFrame->format),
        1, 1//mFrame->sample_aspect_ratio.num, mFrame->sample_aspect_ratio.den
    );
    printf("%s\n", in_args.c_str());

    filter_cxt_ptr_t
        cxt_in, cxt_out;

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

    AVFilterInOut
        *in = avfilter_inout_alloc(),
        *out = avfilter_inout_alloc();

    in->name = av_strdup("in");
    in->filter_ctx = cxt_in.get();
    in->pad_idx = 0;

    out->name = av_strdup("out");
    out->filter_ctx = cxt_out.get();
    out->pad_idx = 0;

    const std::string ratio = std::format("{}/{}", res_dst.w, res_dst.h);
    const std::string args = std::format(
        //"crop=w='if(gt(iw/ih\\,{})\\,ih*{}\\,iw)':h='if(gt(iw/ih\\,{})\\,ih\\,iw*{})',scale={}:{}",
        //ratio, ratio, ratio, ratio, res_dst.w, res_dst.h
        "scale=640:360"
    );
    printf("%s\n", args.c_str());

    ret = avfilter_graph_parse_ptr(filter_graph.get(), args.c_str(), &in, &out, nullptr);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    if(ret < 0)
        throw image_proc_error(false, "graph parse str");

    ret = avfilter_graph_config(filter_graph.get(), nullptr);
    if(ret < 0)
        throw image_proc_error(false, "graph config");

    unique_frame_ptr_t scaled_frame(av_frame_alloc());
    if(!scaled_frame)
        throw image_proc_error(false, "alloc frame");

    scaled_frame->format = pix_fmt;
    scaled_frame->width = res_dst.w;
    scaled_frame->height = res_dst.h;
    // ret = av_frame_get_buffer(scaled_frame.get(), 32);
    // if(ret < 0)
    //     throw image_proc_error(false, "frame get buffer");

    // Encoding itself
    ret = av_buffersrc_add_frame_flags(cxt_in.get(), mFrame.get(), AV_BUFFERSRC_FLAG_KEEP_REF);
    if(ret < 0)
        throw video_proc_error(false, "push frame to filter");
    AVPacket *output_packet = av_packet_alloc();
    if(!output_packet)
        throw video_proc_error(false, "allocate output packet");

    ret = av_buffersink_get_frame(cxt_out.get(), scaled_frame.get());
    if(ret < 0) {
        av_packet_free(&output_packet);
        throw video_proc_error(false, "pull frame from filter");
    }

    ret = avcodec_send_frame(codec_cxt.get(), scaled_frame.get());
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_cxt.get(), output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw video_proc_error(false, "receive packet from encoder");
        }

        output_packet->stream_index = 0;
        if(av_interleaved_write_frame(format_cxt.get(), output_packet) < 0)
            throw video_proc_error(false, "writing packet to encoder");
    }

    av_packet_unref(output_packet);
    av_packet_free(&output_packet);

    if(av_write_trailer(format_cxt.get()))
        throw video_proc_error(false, "write trailer");
}

SlimyTube::ImageConverter::ImageConverter() {

}
