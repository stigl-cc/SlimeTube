#include "Attributes.h"
#include "Comments.h"
#include <RecordManager.h>
#include <drogon/orm/DbClient.h>
#include <models/utils/TimeUtility.h>
#include <models/Settings.h>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpController.h>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/Exception.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <optional>
#include <random>
#include <stdexcept>
#include <mutex>
#include <filesystem>

namespace fs = std::filesystem;
using namespace drogon::orm;
using SlimyTube::Attributes::userId_t;

DbClientPtr SlimyTube::RecordManager::getDbClient() {
    static DbClientPtr db = drogon::app().getDbClient();
    return db;
}

Mapper<Users> SlimyTube::RecordManager::getUserMapper() {
    static Mapper<Users> user_mapper(getDbClient());
    return user_mapper;
}

Mapper<Videos> SlimyTube::RecordManager::getVideosMapper() {
    static Mapper<Videos> video_mapper(getDbClient());
    return video_mapper;
}

Mapper<Comments> SlimyTube::RecordManager::getCommentMapper() {
    static Mapper<Comments> comment_mapper(getDbClient());
    return comment_mapper;
}

std::vector<uint8_t> SlimyTube::RecordManager::createSalt() {
    static const Settings& settings = Settings::getInstance();
    std::vector<uint8_t> salt(settings.salt_len);
    if(!RAND_bytes(salt.data(), salt.size())) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("RAND_bytes Failed");
    }
    return salt;
}

void SlimyTube::RecordManager::createHash(const std::string& pass, std::span<const uint8_t> salt, std::span<uint8_t> hash) {
    if(!PKCS5_PBKDF2_HMAC(
            pass.data(), pass.size(),
            salt.data(), salt.size(),
            400'000, EVP_sha256(),
            hash.size(), hash.data())) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("PBKDF2 Failed");
    }
}

void SlimyTube::RecordManager::registerUser(const std::string& name, const std::string& pass) {
    std::vector<uint8_t> salt = createSalt();

    uint8_t hash[32];
    RecordManager::createHash(pass, salt, hash);

    Users user;
    user.setName(name);
    user.setHash(std::string(reinterpret_cast<char*>(hash), sizeof(hash)));
    user.setSalt(std::string(reinterpret_cast<const char*>(salt.data()), salt.size()));

    getUserMapper().insert(user);
}

void SlimyTube::RecordManager::changeUserPassword(userId_t user_id, const std::string& new_pass) {
    const std::vector<uint8_t> salt = RecordManager::createSalt();

    uint8_t hash[32];
    RecordManager::createHash(new_pass, salt, hash);

    Mapper<Users> user_mapper = RecordManager::getUserMapper();

    Users user = user_mapper.findByPrimaryKey(user_id);
    user.setHash(std::string(reinterpret_cast<const char*>(hash), sizeof(hash)));
    user.setSalt(std::string(reinterpret_cast<const char*>(salt.data()), salt.size()));
    user_mapper.update(user);
}

void SlimyTube::RecordManager::deleteUser(userId_t user_id) {
    DbClientPtr db = RecordManager::getDbClient();
    Mapper<Users> user_mapper(db);
    Users user = user_mapper.findByPrimaryKey(user_id);

    // Delete all videos made by the account
    const std::vector<Videos> videos = user.getVideos(db);
    for(const Videos& video : videos)
        RecordManager::deleteVideo(video);

    // Delete all comments made by the user
    const std::vector<Comments> comments = user.getComments(db);
    for(const Comments& comment : comments)
        RecordManager::deleteComment(comment);

    user_mapper.deleteByPrimaryKey(user_id);
}

std::string SlimyTube::RecordManager::createVideo(uint64_t channel, const std::string& title, const std::string& description) {
    static const Settings& settings = Settings::getInstance();
    Mapper<Videos> video_mapper(getDbClient());

    std::string id;
    do { id = RecordManager::generateVideoId(); } while(videoExists(id));

    Videos video;
    video.setId(id);
    video.setTitle(title);
    video.setDescription(description);
    video.setChannel(channel);
    video.setCreationTime(TimeUtil::GetUnixSecond());
    video.setViews(0);
    video.setState(static_cast<int32_t>(VideoState::Upload));
    video_mapper.insert(video);

    std::error_code ec;
    fs::create_directory(settings.videos_dir / id, ec);
    if(ec) throw std::runtime_error("fs::create_directories failed " + ec.message());
    return id;
}

void SlimyTube::RecordManager::deleteComment(const Comments& comment) {
    getCommentMapper().deleteByPrimaryKey(comment.getPrimaryKey());
}

void SlimyTube::RecordManager::deleteVideo(const Videos& video) {
    static const Settings& settings = Settings::getInstance();
    DbClientPtr db = getDbClient();
    // Delete all comments made on the video
    const std::vector<Comments> comments = video.getComments(db);
    for(const Comments& comment : comments)
        RecordManager::deleteComment(comment);

    Mapper<Videos> video_mapper(db);
    if(video.getValueOfState() == static_cast<int64_t>(VideoState::Encode)) {
        video_mapper.updateBy(
            {Videos::Cols::_state, Videos::Cols::_resolutions},
            Criteria(Videos::Cols::_id, CompareOperator::EQ, video.getValueOfId()),
            static_cast<int32_t>(VideoState::Deleted)
        );
        return;
    }

    std::error_code ec;
    fs::remove_all(settings.videos_dir / video.getValueOfId(), ec);
    if(ec)
        throw std::runtime_error("fs::remove_all failed " + ec.message());
    video_mapper.deleteByPrimaryKey(video.getPrimaryKey());
}

bool SlimyTube::RecordManager::videoExists(const std::string& videoId) {
    return getVideosMapper().count(Criteria(Videos::Cols::_id, CompareOperator::EQ, videoId)) == 1;
}

std::optional<Videos> SlimyTube::RecordManager::getVideoIfExists(const std::string& videoId) {
    Mapper<Videos> video_mapper = getVideosMapper();
    try {
        return video_mapper.findOne(Criteria(Videos::Cols::_id, CompareOperator::EQ, videoId));
    } catch(UnexpectedRows e) {
        return std::nullopt;
    }
}

constexpr std::string_view videoIdCharacters = "abcdefghijklmnopqrstuvwxyz0123456789";
std::string SlimyTube::RecordManager::generateVideoId(size_t size) {
    static std::mutex randomMutex = {};
    static std::random_device dev = {};
    static std::mt19937 rng = std::mt19937(dev());
    static std::uniform_int_distribution<std::mt19937::result_type> distribution(0, videoIdCharacters.size() -1);

    std::lock_guard lk(randomMutex);
    std::string videoId;
    videoId.reserve(size);

    for(int i =0; i < size; i++)
        videoId.push_back(videoIdCharacters[distribution(rng)]);
    return videoId;
}

void SlimyTube::RecordManager::createComment(uint64_t channel, const std::string& video, const std::string& text) {
    Mapper<Comments> comment_mapper(getDbClient());

    Comments comment;
    comment.setChannel(channel);
    comment.setVideo(video);
    comment.setCreationTime(TimeUtil::GetUnixSecond());
    comment.setContent(text);
    comment_mapper.insert(comment);
}
