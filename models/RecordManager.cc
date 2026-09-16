#include <RecordManager.hh>
#include <Settings.hh>

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
    return std::move(salt);
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
