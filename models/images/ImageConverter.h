#pragma once
#include <filesystem>
#include <models/video/VideoTypes.h>

namespace SlimyTube {
    class image_proc_error : public std::runtime_error {
        public:
        bool isUserCaused;
        image_proc_error(bool isUserCaused, const char* const message) throw();
    };

    class ImageConverter {
        private:
        std::filesystem::path mInPath;
        std::filesystem::path mOutPath;

        public:
        ImageConverter(const std::filesystem::path& in_path, const std::filesystem::path& out_path);

        unique_frame_ptr_t mFrame;
        void decode();
        void encode();
    };
};
