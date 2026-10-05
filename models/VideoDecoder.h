#pragma once
#include <models/VideoTypes.h>

namespace SlimyTube {
    class VideoDecoder {
        private:
        struct FormatContextDeleter { void operator()(AVFormatContext*) const; };
        typedef std::unique_ptr<AVFormatContext, FormatContextDeleter> format_cxt_ptr_t;

        private:
        int
            mAudioStreamIndex = -1,
            mVideoStreamIndex = -1;

        format_cxt_ptr_t
            mFormatContext = nullptr;
        const AVCodec
            *mAudioCodec,
            *mVideoCodec;
        codec_cxt_ptr_t
            mAudioCodecContext,
            mVideoCodecContext;

        packet_ptr_t mPacket;
        frame_ptr_t mFrame;

        bool mGetFramePacketAvail = false;
        codec_cxt_ptr_t* mCurrentCodecUsed = nullptr;

        void setupCodec(const AVStream* stream, const AVCodec*& codec, codec_cxt_ptr_t& codecContext);
        void readBlob(const std::string& path);
        void selectStreams();

        public:
        VideoDecoder(const std::string& path);

        void getFrames(video_callback_t videoCallback, audio_callback_t audioCallback);
        void getTimeBase(AVRational& video, AVRational& audio);
        void getMetaData(int& width, int& height, int& bitRate);
    };
}
