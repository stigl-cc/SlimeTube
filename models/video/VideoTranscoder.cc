#include "Attributes.h"
#include <drogon/orm/Criteria.h>
#include <drogon/orm/Mapper.h>
#include <trantor/utils/Logger.h>
#include <endian.h>
#include <memory>
#include <models/RecordManager.h>
#include <models/utils/ResolutionUtility.h>
#include <models/Settings.h>
#include <models/video/VideoTranscoder.h>
#include <models/video/VideoDecoder.h>
#include <models/video/VideoEncoder.h>
#include <models/video/VideoTypes.h>
#include <models/images/ImageConverter.h>

#include <filesystem>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

extern "C" {
#include <libavutil/log.h>
#include <libavcodec/codec.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
}

using namespace drogon::orm;
namespace fs = std::filesystem;

void SlimyTube::VideoTranscoder::onVideoFrame(weak_frame_ptr_t weak_frame) {
    frame_ptr_t frame_ptr = weak_frame.lock();
    if(!frame_ptr)
        throw video_proc_error(false, "frame expired");

    if(mIsFirstFrame) {
        if(mFirstFrameCallback)
            mFirstFrameCallback(frame_ptr);
        mIsFirstFrame = false;
    }

    for(VideoEncoder& encoder : mEncoders)
        encoder.putVideoFrame(frame_ptr);
}

void SlimyTube::VideoTranscoder::onAudioFrame(weak_frame_ptr_t weak_frame) {
    frame_ptr_t frame_ptr = weak_frame.lock();
    if(!frame_ptr)
        throw video_proc_error(false, "frame expired");

    for(VideoEncoder& encoder : mEncoders)
        encoder.putAudioFrame(frame_ptr);
}

SlimyTube::VideoTranscoder::VideoTranscoder(const fs::path& inpath, const fs::path& outpath, const std::vector<VideoTarget>& targets)
    : mDecoder(inpath),
      mEncoders(),
      mOutPath(outpath),
      mFirstFrameCallback(nullptr),
      mIsFirstFrame(true) {

    for(const VideoTarget& target : targets)
        mTargets.push_back(target);
}

void SlimyTube::VideoTranscoder::transcode() {
    AVRational video, audio;
    mDecoder.getTimeBase(video, audio);

    for(const VideoTarget& target : mTargets)
        mEncoders.emplace_back(mOutPath / std::to_string(target.height), target, video, audio);

    mDecoder.getFrames(
        std::bind(&VideoTranscoder::onVideoFrame, this, std::placeholders::_1),
        std::bind(&VideoTranscoder::onAudioFrame, this, std::placeholders::_1)
    );

    for(VideoEncoder& encoder : mEncoders)
        encoder.finish();
}

const SlimyTube::VideoDecoder& SlimyTube::VideoTranscoder::getDecoder() const {
    return mDecoder;
}

void SlimyTube::VideoTranscoder::setFirstFrameCallback(std::function<void(const frame_ptr_t)> f) {
    mFirstFrameCallback = std::move(f);
}

char SlimyTube::VideoTranscoder::logBuffer[2048] = {};
std::mutex SlimyTube::VideoTranscoder::logMutex = {};
std::ofstream SlimyTube::VideoTranscoder::logFileStream = {};
int SlimyTube::VideoTranscoder::logLevel = AV_LOG_INFO;

void SlimyTube::VideoTranscoder::log(void*, int level, const char* msg, va_list va) {
    if(level <= logLevel) {
        std::lock_guard lk(logMutex);
        int len = vsnprintf(logBuffer, sizeof(logBuffer), msg, va);
        logFileStream << logBuffer;
    }
}

void SlimyTube::VideoTranscoder::redirectLog(const std::string& path, int logLevel) {
    std::lock_guard lk(logMutex);

    logFileStream = std::ofstream(path.c_str(), std::ios::app | std::ios::out);
    if(!logFileStream.is_open())
        throw std::runtime_error("Could not open ffmpeg log file!");

    av_log_set_level(logLevel);
    av_log_set_callback(&VideoTranscoder::log);
}

SlimyTube::TranscodeManager::TranscodeManager()
    : mJobs(),
      mWorkerThread(std::thread(std::bind(&TranscodeManager::worker, this))),
      shouldRun(true) {
}

SlimyTube::TranscodeManager::~TranscodeManager() {
    shouldRun = false;
    mConditionVariable.notify_one();
    if(mWorkerThread.joinable())
        mWorkerThread.join();
}

inline std::vector<char> encodeHeights(const std::vector<SlimyTube::VideoTarget>& targets) {
    std::vector<uint16_t> resolutions {};
    for(size_t i =0; i < targets.size(); i++)
        resolutions.push_back(htole16(targets[i].height));

    return std::vector<char>(
        reinterpret_cast<char*>(resolutions.data()),
        reinterpret_cast<char*>(resolutions.data()) + resolutions.size() * sizeof(uint16_t)
    );
}

void SlimyTube::TranscodeManager::editThumbnail(const fs::path& thumbnail_blob, const fs::path& thumbnail) {
    if(!fs::exists(thumbnail_blob))
        return;
    try {
        std::shared_ptr<ImageConverter> thumbnail_converter = std::make_shared<ImageConverter>(thumbnail_blob, thumbnail);
        if(fs::exists(thumbnail_blob)) {
            thumbnail_converter->decode();
            fs::remove(thumbnail_blob);
            thumbnail_converter->encode();
        }
    } catch(const image_proc_error& err) {
        if(err.isUserCaused)
            LOG_WARN << "Thumbnail edit failed (user caused): " << err.what();
        else
            LOG_ERROR << "Thumbnail edit failed: " << err.what();
    } catch(const std::exception& err) {
        LOG_ERROR << "Thumbnail edit failed unexpectedly: " << err.what();
    }
}

static void markFailed(Mapper<Videos>& video_mapper, const std::string& id) {
    try {
        video_mapper.updateBy(
            {Videos::Cols::_state},
            Criteria(Videos::Cols::_id, CompareOperator::EQ, id),
            static_cast<int32_t>(SlimyTube::VideoState::Failed)
        );
    } catch(const std::exception& err) {
        LOG_ERROR << "Could not mark video " << id << " as failed: " << err.what();
    }
}

void SlimyTube::TranscodeManager::worker() {
    const Settings& settings = Settings::getInstance();
    Mapper<Videos> video_mapper = RecordManager::getVideosMapper();

    std::unique_lock<std::mutex> lk(mJobsMutex);
    do {
        while(!mJobs.empty()) {
            const std::string& current_job = mJobs.front();
            lk.unlock();

            const fs::path
                path = settings.videos_dir / current_job,
                thumbnail_blob = path / Paths::thumbnail_blob,
                thumbnail = path / Paths::thumbnail;

            Videos video = video_mapper.findByPrimaryKey(current_job);
            VideoState state = static_cast<VideoState>(video.getValueOfState());
            if(state != VideoState::Encode) {
                if(state == VideoState::Deleted) {
                    RecordManager::deleteVideo(video);
                    lk.lock();
                    mJobs.pop_front();
                    continue;
                }
                if(state == VideoState::Ready) {
                    editThumbnail(thumbnail_blob, thumbnail);
                    lk.lock();
                    mJobs.pop_front();
                }
                continue;
            }

            try {
                const fs::path
                    blob = path / Paths::video_blob;

                VideoTranscoder transcoder(blob, path);

                std::shared_ptr<ImageConverter> thumbnail_converter = std::make_shared<ImageConverter>(thumbnail_blob, thumbnail);
                if(fs::exists(thumbnail_blob)) {
                    thumbnail_converter->decode();
                    fs::remove(thumbnail_blob);
                    thumbnail_converter->encode();
                } else {
                    transcoder.setFirstFrameCallback([&thumbnail_converter](const frame_ptr_t f)-> void {
                        thumbnail_converter->mFrame.reset(av_frame_clone(f.get()));
                        thumbnail_converter->encode();
                    });
                }

                int width, height, bit_rate;
                transcoder.getDecoder().getMetaData(width, height, bit_rate);

                transcoder.mTargets = std::move(ResolutionUtility::GetForVideo({width, height}, bit_rate));
                transcoder.transcode();

                video_mapper.updateBy(
                    {Videos::Cols::_state, Videos::Cols::_resolutions, Videos::Cols::_length},
                    Criteria(Videos::Cols::_id, CompareOperator::EQ, current_job),
                    static_cast<int32_t>(VideoState::Ready),
                    encodeHeights(transcoder.mTargets),
                    transcoder.getDecoder().getLength()
                );

                fs::remove(blob);
            } catch(const video_proc_error& err) {
                if(err.isUserCaused)
                    LOG_WARN << "Video " << current_job << " failed to encode (user caused): " << err.what();
                else
                    LOG_ERROR << "Video " << current_job << " failed to encode: " << err.what();
                markFailed(video_mapper, current_job);
            } catch(const image_proc_error& err) {
                if(err.isUserCaused)
                    LOG_WARN << "Thumbnail for video " << current_job << " failed (user caused): " << err.what();
                else
                    LOG_ERROR << "Thumbnail for video " << current_job << " failed: " << err.what();
                markFailed(video_mapper, current_job);
            } catch(const std::exception& err) {
                // Anything unexpected (filesystem, database, ...) must not kill the worker thread
                LOG_ERROR << "Video " << current_job << " failed unexpectedly: " << err.what();
                markFailed(video_mapper, current_job);
            }

            if(video.getValueOfState() == static_cast<int64_t>(VideoState::Deleted))
                RecordManager::deleteVideo(video);

            lk.lock();
            mJobs.pop_front();
        }

        mConditionVariable.wait(lk);
    } while(shouldRun);

    lk.unlock();
}

void SlimyTube::TranscodeManager::enqueueJob(const std::string& id) {
    {
        std::lock_guard<std::mutex> lk(mJobsMutex);
        mJobs.emplace_back(id);
    }
    mConditionVariable.notify_one();
}

SlimyTube::TranscodeManager& SlimyTube::TranscodeManager::getInstance() {
    static TranscodeManager manager;
    return manager;
}
