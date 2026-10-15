#include "menu/CustomKeys.h"

#include "helpers/InputHelper.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

namespace {

    constexpr const char* cButtonNames[16] = { "A", "B", "X", "Y", "L3", "R3", "L", "R",
                                               "ZL", "ZR", "+", "-", "Left", "Up", "Right", "Down" };

    u16 heldMask() {
        u16 mask = 0;
        for (int i = 0; i < 16; i++)
            if (InputHelper::isButtonHold(nn::hid::NpadButton(i))) mask |= u16(1 << i);
        return mask;
    }

    u16 pressedMask() {
        u16 mask = 0;
        for (int i = 0; i < 16; i++)
            if (InputHelper::isButtonPress(nn::hid::NpadButton(i))) mask |= u16(1 << i);
        return mask;
    }

    int sCaptureSlot = -1;
    bool sIsWaitingRelease = false;
    u16 sCaptureMask = 0;

} // namespace

int customKeySlotOf(const void* field) {
    SettingsMgr::Settings* s = SettingsMgr::getSettings();
    if (!s) return -1;
    if (field == &s->mMenuKey) return OpenMenu;
    if (field == &s->mSavePosKey) return SavePos;
    if (field == &s->mLoadPosKey) return LoadPos;
    if (field == &s->mRestartSceneKey) return RestartScene;
    if (field == &s->mKillSceneKey) return KillScene;
    if (field == &s->mHealMarioKey) return HealMario;
    if (field == &s->mLifeUpKey) return LifeUp;
    if (field == &s->mPrevSceneKey) return PrevScene;
    if (field == &s->mWarpLastCpKey) return WarpLastCp;
    if (field == &s->mTimerStartKey) return TimerStart;
    if (field == &s->mTimerEndKey) return TimerEnd;
    if (field == &s->mTimerResetKey) return TimerReset;
    if (field == &s->mIncTpIndexKey) return IncTpIndex;
    if (field == &s->mDecTpIndexKey) return DecTpIndex;
    if (field == &s->mAddCoinsKey) return AddCoins;
    if (field == &s->mDecCoinsKey) return DecCoins;
    if (field == &s->mIncPatternKey) return IncPattern;
    if (field == &s->mDecPatternKey) return DecPattern;
    return -1;
}

u16 getCustomKey(int slot) {
    SettingsMgr::Settings* s = SettingsMgr::getSettings();
    if (!s || slot < 0 || slot >= CustomKeySlotNum) return 0;
    return s->mCustomKeys[slot];
}

bool isCustomKeyPressed(int slot) {
    u16 mask = getCustomKey(slot);
    if (!mask || sCaptureSlot >= 0) return false;
    return (heldMask() & mask) == mask && (pressedMask() & mask) != 0;
}

void formatCustomKey(u16 mask, char* out, int size) {
    int len = 0;
    out[0] = '\0';
    // Shoulders first, then the rest: "L + ZR + A"
    static constexpr int cOrder[16] = { 6, 7, 8, 9, 10, 11, 4, 5, 12, 13, 14, 15, 0, 1, 2, 3 };
    for (int i : cOrder) {
        if (!(mask & (1 << i))) continue;
        len += snprintf(out + len, size > len ? size - len : 0, "%s%s", len ? " + " : "", cButtonNames[i]);
        if (len >= size) break;
    }
    if (!out[0]) snprintf(out, size, "-");
}

void startCustomKeyCapture(int slot) {
    if (slot < 0 || slot >= CustomKeySlotNum) return;
    sCaptureSlot = slot;
    sIsWaitingRelease = true;
    sCaptureMask = 0;
}

bool isCustomKeyCapturing(int slot) {
    return sCaptureSlot >= 0 && (slot < 0 || slot == sCaptureSlot);
}

bool updateCustomKeyCapture() {
    if (sCaptureSlot < 0) return false;
    u16 held = heldMask();
    if (sIsWaitingRelease) {
        // A (which started the recording) must be released first
        if (!held) sIsWaitingRelease = false;
        return true;
    }
    if (held) {
        sCaptureMask |= held;
    } else if (sCaptureMask) {
        if (SettingsMgr::Settings* s = SettingsMgr::getSettings()) s->mCustomKeys[sCaptureSlot] = sCaptureMask;
        sCaptureSlot = -1;
    }
    return true;
}

} // namespace btt
