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
    class RecordManager {
        public:
        static DbClientPtr getDbClient();
        static Mapper<Users> getUserMapper();
        static Mapper<Videos> getVideosMapper();

        static std::vector<uint8_t> createSalt();
        static void createHash(const std::string& pass, std::span<const uint8_t> salt, std::span<uint8_t> hash);
        static bool nameExists(const std::string& name, Mapper<Users>& mapper);

        static bool videoExists(const std::string& id);
        static std::optional<Videos> getVideoIfExists(const std::string& id);
    };
};
