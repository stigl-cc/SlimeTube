#pragma once
#include <memory>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

namespace SlimyTube {
    class VideoEncoder {
        private:
        struct InputFormatContextDeleter {  void operator()(AVFormatContext*); };
        struct OutputFormatContextDeleter { void operator()(AVFormatContext*); };

        struct CodecDeleter { void operator()(AVCodec*); };
        struct CodecContextDeleter { void operator()(AVCodecContext*); };

        std::unique_ptr<AVFormatContext, InputFormatContextDeleter> inputFormatContext;
        std::unique_ptr<AVFormatContext, OutputFormatContextDeleter> outputFormatContext;

        void readBlob(const std::string& path);
        void setupCodec(const AVStream* stream);
        void decode();

        public:
        VideoEncoder();
    };
};
