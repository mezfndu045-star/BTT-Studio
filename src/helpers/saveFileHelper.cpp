#include "helpers/saveFileHelper.h"

#include "nn/fs/fs_directories.h"
#include "nn/fs/fs_mount.h"
#include "nn/os.h"

#include "sead/heap/seadHeapMgr.h"

#include <cstddef>
#include <cstring>

#include "helpers/fsHelper.h"
#include "logger.h"
#include "src/settings/SettingsMgr.h"

SEAD_SINGLETON_DISPOSER_IMPL(SaveFileHelper);

// Settings are written as soon as one changes (checked every second), so they are kept even when the emulator is
// closed without leaving the game
void SaveFileHelper::ThreadSave() {
    while (true) {
        SaveFileHelper::instance()->saveSettingsIfChanged();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
    }
}

void SaveFileHelper::saveSettingsIfChanged() {
    const auto* settings = &btt::SettingsMgr::instance()->mSettings;
    if (mLastSavedSettings && memcmp(mLastSavedSettings, settings, sizeof(btt::SettingsMgr::Settings)) == 0) return;
    saveSettings();
}

void SaveFileHelper::init(sead::Heap* heap) {
    mHeap = heap;
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    mLastSavedSettings = heap->tryAlloc(sizeof(btt::SettingsMgr::Settings), 8);
    al::FunctorV0M functor(this, &SaveFileHelper::ThreadSave);
    mSaveThread = new al::AsyncFunctorThread("Save Thread", functor, 0, 0x20000, {});
}

bool SaveFileHelper::saveSettings() {
    static int sFailCount = 0;
    if (!FsHelper::isDirExist("sd:/BTT-Studio")) {
        nn::Result dirResult = nn::fs::CreateDirectory("sd:/BTT-Studio");
        if (dirResult.IsFailure() && !FsHelper::isDirExist("sd:/BTT-Studio")) {
            // The SD card may not be mounted yet: mount it and try again on the next save
            int mountResult = (int)nn::fs::MountSdCardForDebug("sd");
            if (sFailCount++ < 5)
                Logger::instance()->log(
                    Logger::LogType::LogErr, "Settings: cannot create sd:/BTT-Studio (0x%x), sd mount: 0x%x",
                    dirResult.GetInnerValueForDebug(), mountResult
                );
            return false;
        }
    }

    // Copy first so a setting changed during the write is saved on the next check
    if (mLastSavedSettings)
        memcpy(mLastSavedSettings, &btt::SettingsMgr::instance()->mSettings, sizeof(btt::SettingsMgr::Settings));
    nn::Result result = FsHelper::writeFileToPath(
        mLastSavedSettings ? mLastSavedSettings : reinterpret_cast<void*>(&btt::SettingsMgr::instance()->mSettings),
        sizeof(btt::SettingsMgr::Settings), mSettingsPath
    );
    if (result.IsFailure()) {
        if (sFailCount++ < 5)
            Logger::instance()->log(
                Logger::LogType::LogErr, "Settings: cannot write %s (0x%x)", mSettingsPath, result.GetInnerValueForDebug()
            );
        if (mLastSavedSettings) memset(mLastSavedSettings, 0, sizeof(btt::SettingsMgr::Settings));
        return false;
    }
    Logger::instance()->log(Logger::LogType::LogInfo, "Settings saved to %s", mSettingsPath);
    return true;
}

void SaveFileHelper::loadSettings(sead::Heap* heap) {
    if (!FsHelper::isFileExist(mSettingsPath)) {
        saveSettings();
        return;
    }

    FsHelper::LoadData data;
    data.path = mSettingsPath;
    FsHelper::loadFileFromPath(data, heap);
    if (!data.buffer) {
        Logger::instance()->log(Logger::LogType::LogErr, "Failed to load settings from %s", mSettingsPath);
        return;
    }
    // Settings appended at the end of the struct keep their defaults when loading an older, smaller file
    if (data.bufSize > sizeof(btt::SettingsMgr::Settings) || data.bufSize < sizeof(btt::SettingsMgr::Settings::Version)) {
        Logger::instance()->log(
            Logger::LogType::LogErr, "Invalid settings size: %zu, expected: %zu", data.bufSize,
            sizeof(btt::SettingsMgr::Settings)
        );
        return;
    }
    btt::SettingsMgr::Settings* configData = reinterpret_cast<btt::SettingsMgr::Settings*>(data.buffer);

    if (strcmp(configData->Version, btt::SettingsMgr::instance()->mSettings.Version) != 0) {
        Logger::instance()->log(
            Logger::LogType::LogWarn, "Version mismatch: %s != %s, resetting Settings", configData->Version,
            btt::SettingsMgr::instance()->mSettings.Version
        );
        return;
    }

    memcpy(&btt::SettingsMgr::instance()->mSettings, data.buffer, data.bufSize);
    Logger::instance()->log(Logger::LogType::LogInfo, "Settings loaded from %s", mSettingsPath);
    // Nothing to write until a setting changes
    if (mLastSavedSettings)
        memcpy(mLastSavedSettings, &btt::SettingsMgr::instance()->mSettings, sizeof(btt::SettingsMgr::Settings));
}

void SaveFileHelper::saveTeleport(btt::Menu::TpState* states, size_t count) {
    if (!FsHelper::isDirExist("sd:/BTT-Studio")) {
        nn::fs::CreateDirectory("sd:/BTT-Studio");
    }

    FsHelper::writeFileToPath(reinterpret_cast<void*>(states), sizeof(btt::Menu::TpState) * count, mtpPath);
}

void SaveFileHelper::loadTeleport(btt::Menu::TpState* states, size_t count, sead::Heap* heap) {
    if (!FsHelper::isFileExist(mtpPath)) {
        saveTeleport(states, count);
        return;
    }

    FsHelper::LoadData data;
    data.path = mtpPath;
    FsHelper::loadFileFromPath(data, heap);

    if (data.buffer != nullptr && data.bufSize == sizeof(btt::Menu::TpState) * count) {
        btt::Menu::TpState* loadedStates = reinterpret_cast<btt::Menu::TpState*>(data.buffer);
        for (size_t i = 0; i < count; ++i) {
            states[i] = loadedStates[i];
        }
    }
}
