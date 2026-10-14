#include "menu/PhotoTour.h"

#include "nn/fs/fs_directories.h"
#include "nn/fs/fs_files.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/GameDataHolder.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "helpers/fsHelper.h"
#include "helpers/getHelper.h"
#include "logger.h"
#include "menu/PhotoTourList.h"
#include "menu/stage_warp.h"
#include "game/System/GameDataFunction.h"
#include "al/Library/Base/StringUtil.h"

namespace btt {

namespace {

    constexpr const char* cStartPath = "sd:/BTT-Studio/tour_start.txt";
    constexpr const char* cStopPath = "sd:/BTT-Studio/tour_stop.txt";
    constexpr const char* cShotPath = "sd:/BTT-Studio/tour_shot.txt";
    constexpr const char* cAckPath = "sd:/BTT-Studio/tour_ack.txt";
    constexpr const char* cDonePath = "sd:/BTT-Studio/tour_done.txt";

    // Frames after the new stage appears before the picture (Mario coming out of the pipe / door, title text fading)
    constexpr int cSettleFrames = 330;
    constexpr int cSceneTimeoutFrames = 60 * 40;
    constexpr int cAckTimeoutFrames = 60 * 20;

    enum class State { Idle, Warp, WaitScene, WaitAck };

    State sState = State::Idle;
    int sShot = 0;
    int sTimer = 0;
    int sPollTimer = 0;
    int sWarpGeneration = 0;

    void writeText(const char* path, const char* text) {
        FsHelper::writeFileToPath((void*)text, strlen(text), path);
    }

    int readStartIndex() {
        FsHelper::LoadData data = { .path = cStartPath };
        FsHelper::loadFileFromPath(data, nullptr);
        int index = 0;
        if (data.buffer) {
            char buf[16] = {};
            memcpy(buf, data.buffer, data.bufSize < 15 ? data.bufSize : 15);
            index = atoi(buf);
            free(data.buffer);
        }
        return index;
    }

    void warp(const PhotoTourShot& shot, HakoniwaSequence* gameSeq) {
        // Every picture in the kingdom's last scenario, where everything is unlocked
        warpToStageSafe(shot.stage, shot.label, getLastScenarioOfStage(shot.stage));
    }

} // namespace

bool isPhotoTourRunning() {
    return sState != State::Idle;
}

void updatePhotoTour() {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
    StageScene* stageScene = helpers::tryGetStageScene(gameSeq);

    // Files are checked a few times per second only
    bool isPoll = ++sPollTimer >= 10;
    if (isPoll) sPollTimer = 0;

    if (sState == State::Idle) {
        if (isPoll && FsHelper::isFileExist(cStartPath)) {
            sShot = readStartIndex();
            nn::fs::DeleteFile(cStartPath);
            if (FsHelper::isFileExist(cDonePath)) nn::fs::DeleteFile(cDonePath);
            sState = State::Warp;
            Logger::instance()->log(Logger::LogType::LogInfo, "PhotoTour: start at %d / %d", sShot, cPhotoTourShotNum);
        }
        return;
    }
    if (isPoll && FsHelper::isFileExist(cStopPath)) {
        nn::fs::DeleteFile(cStopPath);
        sState = State::Idle;
        Logger::instance()->log(Logger::LogType::LogInfo, "PhotoTour: stopped at %d", sShot);
        return;
    }

    switch (sState) {
    case State::Warp:
        if (sShot >= cPhotoTourShotNum) {
            writeText(cDonePath, "done");
            sState = State::Idle;
            Logger::instance()->log(Logger::LogType::LogInfo, "PhotoTour: done");
            return;
        }
        if (!gameSeq || !stageScene || !stageScene->mIsAlive) return;
        sWarpGeneration = getStageSceneGeneration();
        Logger::instance()->log(
            Logger::LogType::LogInfo, "PhotoTour: %d %s -> %s (%s)", sShot, sPhotoTourShots[sShot].shot,
            sPhotoTourShots[sShot].stage, sPhotoTourShots[sShot].label
        );
        warp(sPhotoTourShots[sShot], gameSeq);
        sTimer = 0;
        sState = State::WaitScene;
        return;
    case State::WaitScene: {
        sTimer++;
        // The shot's stage itself, not the kingdom loaded first by warpToStageSafe
        bool isNewScene = stageScene && stageScene->mIsAlive && getStageSceneGeneration() != sWarpGeneration &&
                          !isStageWarpPending() &&
                          al::isEqualString(GameDataFunction::getCurrentStageName(gameSeq->mGameDataHolderAccessor),
                                            sPhotoTourShots[sShot].stage);
        static int sSettle = 0;
        if (!isNewScene) {
            sSettle = 0;
            if (sTimer > cSceneTimeoutFrames) {
                Logger::instance()->log(Logger::LogType::LogErr, "PhotoTour: %s did not load", sPhotoTourShots[sShot].shot);
                sShot++;
                sState = State::Warp;
            }
            return;
        }
        if (++sSettle < cSettleFrames) return;
        sSettle = 0;
        writeText(cShotPath, sPhotoTourShots[sShot].shot);
        sTimer = 0;
        sState = State::WaitAck;
        return;
    }
    case State::WaitAck:
        sTimer++;
        if (isPoll && FsHelper::isFileExist(cAckPath)) {
            nn::fs::DeleteFile(cAckPath);
            sShot++;
            sState = State::Warp;
        } else if (sTimer > cAckTimeoutFrames) {
            Logger::instance()->log(Logger::LogType::LogErr, "PhotoTour: no screenshot for %s", sPhotoTourShots[sShot].shot);
            sShot++;
            sState = State::Warp;
        }
        return;
    default: return;
    }
}

} // namespace btt
