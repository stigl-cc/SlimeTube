#pragma once
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <models/Users.h>
#include <models/Videos.h>
#include <models/Attributes.h>

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
        static Mapper<Comments> getCommentMapper();

        static std::vector<uint8_t> createSalt();
        static void createHash(const std::string& pass, std::span<const uint8_t> salt, std::span<uint8_t> hash);

        static inline bool nameExists(const std::string& name, Mapper<Users>& mapper) {
            return mapper.count(drogon::orm::Criteria(Users::Cols::_name, drogon::orm::CompareOperator::EQ, name)) == 1;
        }

        static void registerUser(const std::string& name, const std::string& pass);

        static void changeUserName(Attributes::userId_t userId, const std::string& newName) {
            getUserMapper().updateBy({ Users::Cols::_name }, { Users::Cols::_id, drogon::orm::CompareOperator::EQ, userId }, newName);
        }

        static void changeUserPassword(Attributes::userId_t userId, const std::string& newPass);
        static void deleteUser(Attributes::userId_t userId);

        static std::string createVideo(uint64_t channel, const std::string& title, const std::string& description);
        static void deleteVideo(const Videos&);

        static bool videoExists(const std::string& videoId);
        static std::optional<Videos> getVideoIfExists(const std::string& videoId);

        static inline void changeVideoTitle(const std::string& videoId, const std::string& newTitle) {
            getVideosMapper().updateBy({ Videos::Cols::_title }, drogon::orm::Criteria(Videos::Cols::_id, drogon::orm::CompareOperator::EQ, videoId), newTitle);
        }

        static inline void changeVideoDescription(const std::string& videoId, const std::string& newDescription) {
            getVideosMapper().updateBy({ Videos::Cols::_description }, drogon::orm::Criteria(Videos::Cols::_id, drogon::orm::CompareOperator::EQ, videoId), newDescription);
        }

        static std::string generateVideoId(size_t size = 6);

        static void createComment(uint64_t channel, const std::string& video, const std::string& text);
        static void deleteComment(const Comments& comment);
    };
};
