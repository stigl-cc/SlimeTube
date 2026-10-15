#pragma once
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <models/Users.h>
#include <models/Videos.h>
#include <models/Attributes.hh>

using namespace drogon_model::SlimyTube;
using drogon::orm::DbClientPtr,
    drogon::orm::Mapper;

namespace SlimyTube {
    enum class VideoState {
        Upload = 0, // blob present
        Encode,     // blob undefined, processing/queued for processing
        Ready,
        Failed,
        Deleted,
    };

    class RecordManager {
        public:
        static DbClientPtr getDbClient();
        static Mapper<Users> getUserMapper();
        static Mapper<Videos> getVideosMapper();

        static std::vector<uint8_t> createSalt();
        static void createHash(const std::string& pass, std::span<const uint8_t> salt, std::span<uint8_t> hash);
        static bool nameExists(const std::string& name, Mapper<Users>& mapper);

        static std::string createVideo(uint64_t channel, const std::string& title, const std::string& description);
        static void deleteVideo(const Videos&);

        static bool videoExists(const std::string& id);
        static std::optional<Videos> getVideoIfExists(const std::string& id);

        static std::string generateVideoId(size_t size = 6);
    };
};
