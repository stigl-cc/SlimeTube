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
        unique_frame_ptr_t mFrame;
        void scale();
        void decode(const std::filesystem::path& inputPath);
        void encode(const std::filesystem::path& outputPath);

        public:
        ImageConverter();
    };
};
