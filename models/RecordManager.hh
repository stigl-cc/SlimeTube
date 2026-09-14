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

        static void createSalt(const std::span<uint8_t, 16> salt);
        static void createHash(const std::string& pass, const std::span<const uint8_t, 16> salt, const std::span<uint8_t, 32>& hash);
        static bool nameExists(const std::string& name, Mapper<Users>& mapper);
    };
};
