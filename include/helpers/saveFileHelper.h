#pragma once

#include "al/Library/Thread/AsyncFunctorThread.h"

#include "sead/heap/seadDisposer.h"

#include "menu/Menu.h"

class SaveFileHelper {
    SEAD_SINGLETON_DISPOSER(SaveFileHelper)

private:
    const char mSettingsPath[28] = "sd:/BTT-Studio/Settings.bin";
    const char mtpPath[35] = "sd:/BTT-Studio/Teleport-States.bin";
    sead::Heap* mHeap = nullptr;
    // Copy of the settings last written to the SD card, to save again as soon as one changes
    void* mLastSavedSettings = nullptr;

    void ThreadSave();

public:
    SaveFileHelper() = default;

    void init(sead::Heap* heap);
    al::AsyncFunctorThread* mSaveThread = nullptr;

    bool saveSettings();
    void saveSettingsIfChanged();
    void loadSettings(sead::Heap* heap);
    void saveTeleport(btt::Menu::TpState* states, size_t count);
    void loadTeleport(btt::Menu::TpState* states, size_t count, sead::Heap* heap);
};
