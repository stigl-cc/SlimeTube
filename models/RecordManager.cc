#include <RecordManager.h>
#include <Settings.hh>

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/HttpController.h>
#include <drogon/orm/Criteria.h>

#include <drogon/orm/Exception.h>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <optional>
#include <random>
#include <stdexcept>

using namespace drogon::orm;

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

std::vector<uint8_t> SlimyTube::RecordManager::createSalt() {
    std::vector<uint8_t> salt(SlimyTube::Settings::getInstance().salt_len);
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

bool SlimyTube::RecordManager::nameExists(const std::string& name, Mapper<Users>& mapper) {
    // username already taken
    return mapper.count(Criteria(Users::Cols::_name, CompareOperator::EQ, name)) == 1;
}

void SlimyTube::RecordManager::deleteVideo(const Videos& video) {
    // TODO: FINISH
    LOG_ERROR << "Feature not implemented! deleteVideo";
    getVideosMapper().deleteByPrimaryKey(video.getPrimaryKey());
}

bool SlimyTube::RecordManager::videoExists(const std::string& id) {
    return getVideosMapper().count(Criteria(Videos::Cols::_id, CompareOperator::EQ, id)) == 1;
}

std::optional<Videos> SlimyTube::RecordManager::getVideoIfExists(const std::string& id) {
    Mapper<Videos> video_mapper = getVideosMapper();
    try {
        return video_mapper.findOne(Criteria(Videos::Cols::_id, CompareOperator::EQ, id));
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
