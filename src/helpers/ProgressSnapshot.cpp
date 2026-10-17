#include "helpers/ProgressSnapshot.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/MapObj/ChangeStageInfo.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameDataHolderAccessor.h"

#include "src/settings/SettingsMgr.h"
#include "menu/DebugCommands.h"

// sead::RamWriteStream (Modes: 0 = binary) and the GameDataFile save code (same in 1.0 and 1.3)
extern "C" void ProgressSnapshot_RamWriteStream_ctor(void* self, void* buffer, u32 size, int mode)
    asm("_ZN4sead14RamWriteStreamC2EPvjNS_6Stream5ModesE");
extern "C" void ProgressSnapshot_RamWriteStream_dtor(void* self) asm("_ZN4sead14RamWriteStreamD1Ev");
extern "C" void ProgressSnapshot_writeToStream(const void* file, void* stream, void* heap)
    asm("_ZNK12GameDataFile13writeToStreamEPN4sead11WriteStreamEPNS0_4HeapE");
extern "C" void ProgressSnapshot_initializeData(void* file) asm("_ZN12GameDataFile14initializeDataEv");
extern "C" bool ProgressSnapshot_tryReadByamlData(void* file, const u8* data) asm("_ZN12GameDataFile16tryReadByamlDataEPKh");
extern "C" const char* ProgressSnapshot_getNextPlayerStartId(const void* holder)
    asm("_ZNK14GameDataHolder20getNextPlayerStartIdEv");

namespace btt {

void restoreVisitScenario();  // SettingsHooks.cpp

namespace {

    // A save is about 100-130 KB ([s32 size][byml]); the game itself allows up to 2 MB
    constexpr u32 cSnapshotSize = 0x80000;
    u8* sSnapshot = nullptr;
    bool sIsSnapshotValid = false;

    // GameDataHolder: +0x20 GameDataFile (current file), +0x90 heap used by the game to write the save
    void* fileOf(GameDataHolder* holder) { return *reinterpret_cast<void**>(reinterpret_cast<u8*>(holder) + 0x20); }
    void* heapOf(GameDataHolder* holder) { return *reinterpret_cast<void**>(reinterpret_cast<u8*>(holder) + 0x90); }

    // Checkpoint flag positions (CheckpointFlag::init -> GameDataFile::registerCheckpointTrans), not in the save data:
    // GameDataFile +0x9e8 / 1.3 +0xa30 -> 20 worlds -> 16 entries of 0x148 bytes, position at +0x134. initializeData
    // clears them and only the main stage of a kingdom registers them again, so they are kept across a restore (else
    // Restart Scene in a sub-area loses the flags, All Checkpoints included).
#if BTT_V130
    constexpr int cCheckpointTableOffset = 0xa30;
#else
    constexpr int cCheckpointTableOffset = 0x9e8;
#endif
    constexpr int cCheckpointWorldNum = 20, cCheckpointNum = 16, cCheckpointEntrySize = 0x148, cCheckpointTransOffset = 0x134;
    float sCheckpointTrans[cCheckpointWorldNum][cCheckpointNum][3];

    float* checkpointTrans(void* file, int world, int index) {
        u8** worlds = *reinterpret_cast<u8***>(static_cast<u8*>(file) + cCheckpointTableOffset);
        if (!worlds || !worlds[world]) return nullptr;
        return reinterpret_cast<float*>(worlds[world] + index * cCheckpointEntrySize + cCheckpointTransOffset);
    }

    void saveCheckpointTrans(void* file) {
        for (int w = 0; w < cCheckpointWorldNum; w++)
            for (int i = 0; i < cCheckpointNum; i++) {
                float* t = checkpointTrans(file, w, i);
                memcpy(sCheckpointTrans[w][i], t ? t : (const float[3]){ 0, 0, 0 }, sizeof(float) * 3);
            }
    }

    void restoreCheckpointTrans(void* file) {
        for (int w = 0; w < cCheckpointWorldNum; w++)
            for (int i = 0; i < cCheckpointNum; i++) {
                float* t = checkpointTrans(file, w, i);
                if (t && t[0] == 0 && t[1] == 0 && t[2] == 0) memcpy(t, sCheckpointTrans[w][i], sizeof(float) * 3);
            }
    }

    bool isMoonRefresh() {
        SettingsMgr::Settings* s = SettingsMgr::getSettings();
        return s && s->mIsEnableMoonRefresh;
    }

    // Writes the progress of the current file into buffer ([s32 size][byml]), true if complete
    bool writeSnapshot(GameDataHolder* holder, u8* buffer) {
        void* file = fileOf(holder);
        void* heap = heapOf(holder);
        if (!file || !heap || !buffer) return false;
        memset(buffer, 0, 8);
        // Scratch heap: the game empties it before each save write (GameDataHolder::writeToSaveDataBuffer, vtable
        // +0x58 sead::FrameHeap::freeAll), otherwise it fills up and the write aborts in FrameHeap::tryAlloc
        using FreeAllFn = void (*)(void*);
        (*reinterpret_cast<FreeAllFn**>(heap))[0x58 / 8](heap);
        alignas(16) u8 stream[0x100] = {};
        ProgressSnapshot_RamWriteStream_ctor(stream, buffer, cSnapshotSize, 0);
        ProgressSnapshot_writeToStream(file, stream, heap);
        ProgressSnapshot_RamWriteStream_dtor(stream);
        u32 size = 0;
        memcpy(&size, buffer, sizeof(size));
        // A full byml ("BY" / "YB" header) that fit in the buffer
        return size > 16 && size + 4 <= cSnapshotSize &&
               ((buffer[4] == 'B' && buffer[5] == 'Y') || (buffer[4] == 'Y' && buffer[5] == 'B'));
    }

    // Odyssey travel snapshots (Moon Refresh / Gray Moon Refresh): progress before the first travel to each kingdom
    // (a few kingdoms: an IL goes back and forth between two), each one in a buffer of its own size
    struct TravelSnapshot {
        char stage[64];
        u8* data;  // [s32 size][byml]
        u32 age;
    };
    constexpr int cTravelSnapshotNum = 4;
    TravelSnapshot sTravelSnapshots[cTravelSnapshotNum] = {};
    u32 sTravelAge = 0;

} // namespace

void takeProgressSnapshot(GameDataHolder* holder) {
    sIsSnapshotValid = false;
    if (!holder || !isMoonRefresh()) return;
    if (!sSnapshot) sSnapshot = static_cast<u8*>(malloc(cSnapshotSize));
    sIsSnapshotValid = writeSnapshot(holder, sSnapshot);
}

bool takeOrRestoreTravelSnapshot(GameDataHolder* holder, const char* stageName) {
    if (!holder || !stageName) return false;
    void* file = fileOf(holder);
    if (!file) return false;
    for (TravelSnapshot& snap : sTravelSnapshots) {
        if (!snap.data || strcmp(snap.stage, stageName) != 0) continue;
        snap.age = ++sTravelAge;
        saveCheckpointTrans(file);
        ProgressSnapshot_initializeData(file);
        bool isRead = ProgressSnapshot_tryReadByamlData(file, snap.data + 4);
        restoreCheckpointTrans(file);
        return isRead;
    }
    // Written in the work buffer, then copied into a slot (a free one, or the oldest)
    static u8* sWork = nullptr;
    if (!sWork) sWork = static_cast<u8*>(malloc(cSnapshotSize));
    if (!writeSnapshot(holder, sWork)) return false;
    u32 size = 0;
    memcpy(&size, sWork, sizeof(size));
    TravelSnapshot* slot = &sTravelSnapshots[0];
    for (TravelSnapshot& snap : sTravelSnapshots) {
        if (!snap.data) {
            slot = &snap;
            break;
        }
        if (snap.age < slot->age) slot = &snap;
    }
    if (slot->data) free(slot->data);
    slot->data = static_cast<u8*>(malloc(size + 4));
    if (!slot->data) return false;
    memcpy(slot->data, sWork, size + 4);
    snprintf(slot->stage, sizeof(slot->stage), "%s", stageName);
    slot->age = ++sTravelAge;
    return false;
}

const char* ProgressSnapshot_getRestartStartId(GameDataHolder* holder) {
    return holder ? ProgressSnapshot_getNextPlayerStartId(holder) : nullptr;
}

bool restoreProgressSnapshot(GameDataHolder* holder) {
    if (!holder || !sIsSnapshotValid || !isMoonRefresh()) return false;
    void* file = fileOf(holder);
    if (!file) return false;
    // The stage and entrance live in the file that is about to be reset: copy them first
    char stage[128] = {}, startId[128] = {};
    const char* curStage = GameDataFunction::getCurrentStageName(GameDataHolderAccessor(holder));
    const char* curStart = ProgressSnapshot_getNextPlayerStartId(holder);
    if (!curStage || !curStage[0]) return false;
    snprintf(stage, sizeof(stage), "%s", curStage);
    snprintf(startId, sizeof(startId), "%s", curStart && curStart[0] ? curStart : "start");
    if (isDebugTrace()) dbgLog("DBG: restore snapshot stage %s start %s", stage, startId);

    saveCheckpointTrans(file);
    ProgressSnapshot_initializeData(file);
    if (!ProgressSnapshot_tryReadByamlData(file, sSnapshot + 4)) return false;
    restoreCheckpointTrans(file);
    restoreVisitScenario();

    ChangeStageInfo info(holder, startId, stage, false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
    holder->changeNextStage(&info, 0);
    return true;
}

} // namespace btt
