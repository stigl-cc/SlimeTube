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
        public: unique_frame_ptr_t mFrame;

        public:
        ImageConverter();
        void decode(const std::filesystem::path& inputPath);
        void encode(const std::filesystem::path& outputPath);
    };
};
