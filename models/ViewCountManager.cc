#include "Attributes.h"
#include <drogon/HttpAppFramework.h>
#include <models/ViewCountManager.h>

#include <drogon/orm/DbClient.h>
#include <fstream>
#include <json/reader.h>
#include <json/value.h>
#include <mutex>
#include <models/RecordManager.h>
#include <models/Videos.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>

using namespace drogon::orm;
using namespace drogon_model::SlimyTube;

SlimyTube::ViewCountManager::ViewCountManager() {
    mViewQueue.reserve(8192);
}

void SlimyTube::ViewCountManager::addView(const std::string& id, uint32_t amount) {
    size_t id_hash = std::hash<std::string>{}(id);

    Stripe& stripe = videoMutexes[id_hash % mutexCount];
    std::lock_guard<std::mutex> lk(stripe.mutex);
    stripe.videos.insert(id);

    mViewQueue[id] += amount;
}

void SlimyTube::ViewCountManager::processViews() {
    DbClientPtr db = RecordManager::getDbClient();

    for(Stripe& stripe : videoMutexes) {
        std::lock_guard<std::mutex> lk(stripe.mutex);
        for(const std::string& video : stripe.videos) {
            db->execSqlSync("UPDATE videos SET views = views + $1 WHERE id = $2;", mViewQueue[video], video);
            mViewQueue.erase(video);
        }
        stripe.videos.clear();
    }
}

void SlimyTube::ViewCountManager::restoreState() {
    std::ifstream ifs(mCachePath, std::ios_base::in);
    if(!ifs.is_open())
        return;

    Json::Value root;
    JSONCPP_STRING errs;
    if(!Json::parseFromStream(Json::CharReaderBuilder(), ifs, &root, &errs)) {
        LOG_WARN << "Old views could not be parsed, skipping " << errs;
        return;
    }

    Json::Value id, value;
    for(Json::Value video : root) {
        id = video[ViewCache::id];
        value = video[ViewCache::count];
        if(!id.isNull() && !value.isNull())
            addView(id.asString(), value.asInt());
    }

    drogon::app().getLoop()
        ->runAfter(1, std::bind(&ViewCountManager::processViews, this));
}

void SlimyTube::ViewCountManager::saveState() {
    if(mCachePath.empty())
        return;

    std::ofstream ofs(mCachePath, std::ios_base::trunc | std::ios_base::out);
    if(!ofs.is_open())
        return;

    Json::Value unwritten_views = {};
    for(const std::pair<std::string, uint32_t> videos : mViewQueue) {
        Json::Value video;
        video[ViewCache::id] = std::move(videos.first);
        video[ViewCache::count] = videos.second;
        unwritten_views.append(video);
    }

    ofs << unwritten_views.toStyledString();
}

void SlimyTube::ViewCountManager::setCachePath(std::string cachePath) {
    mCachePath = std::move(cachePath);
    restoreState();
}

SlimyTube::ViewCountManager::~ViewCountManager() {
    // database not alive at this point and it is not easy to interject before its shutdown
    // we save the undatabased views in a json file to be saved at start
    saveState();
}

SlimyTube::ViewCountManager& SlimyTube::ViewCountManager::getInstance() {
    static ViewCountManager instance;
    return instance;
}
