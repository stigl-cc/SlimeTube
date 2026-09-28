#pragma once
#include <memory>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

namespace SlimyTube {
    class VideoDecoder {
        private:
        struct FormatContextDeleter {  void operator()(AVFormatContext*); };

        struct CodecDeleter { void operator()(const AVCodec*); };
        struct CodecContextDeleter { void operator()(AVCodecContext*); };

        typedef std::unique_ptr<const AVCodec, CodecDeleter> codec_ptr_t;
        typedef std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_cxt_ptr_t;

        std::unique_ptr<AVFormatContext, FormatContextDeleter> inputFormatContext = nullptr;

        int
            audioStreamIndex = -1,
            videoStreamIndex = -1;

        codec_ptr_t
            audioCodec,
            videoCodec;

        codec_cxt_ptr_t
            audioCodecContext,
            videoCodecContext;

        void setupCodec(const AVStream* stream, codec_ptr_t& codec, codec_cxt_ptr_t& codecContext);

        void readBlob(const std::string& path);

        void selectStreams();
        void decode();

        public:
        VideoDecoder();
    };

    class VideoEncoder {
        struct OutputFormatContextDeleter { void operator()(AVFormatContext*); };

        std::unique_ptr<AVFormatContext, OutputFormatContextDeleter> outputFormatContext = nullptr;
    };

};
