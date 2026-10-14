#pragma once

#include "sead/heap/seadDisposer.h"

#include "imgui.h"
#include "menu/Timer.h"

namespace btt {

#define SETTING(NAME) bool mIsEnable##NAME
#define VERSION "1.3.1"
#ifdef BTTDEBUG
# define SUBVER "-dev"
#else
# define SUBVER ""
#endif

class SettingsMgr {
    SEAD_SINGLETON_DISPOSER(SettingsMgr)

private:
    sead::Heap* mHeap;

public:
    SettingsMgr() = default;

    enum class InputDisplayColor : u8 {
        None,
        White,
        Gray,
        Black,
        Red,
        Blue,
        Yellow,
        Green,
        Orange,
        Purple,
        Pink,
        LightBlue,
        Multicolor
    };
    enum class SettingsHotkey : u8 { Disabled, None, L, R, ZL, ZR, LR, LZL, LZR, RZL, RZR };
    enum class TpKey : u8 {
        DLeft = 0,
        DRight,
        DDown,
        DUp,
        L_DLeft,
        L_DRight,
        L_DDown,
        L_DUp,
        ZL_DLeft,
        ZL_DRight,
        ZL_DDown,
        ZL_DUp,
        R_DLeft,
        R_DRight,
        LStick,
        RStick,
        L_LStick,
        L_RStick,
        ZL_ZR,
        Disabled
    };

    struct __attribute__((packed)) Settings {
        char Version[0x8] = VERSION;
        SETTING(MoonRefresh);
        SETTING(GrayMoonRefresh);
        SETTING(NoDamage);
        SETTING(DisableMoonLock);
        SETTING(AlwaysManualCutscene);
        SETTING(DisableAutoSave);
        SETTING(DisableMusic);
        SETTING(AlwaysCheckpoints);
        SETTING(RefreshWarpText);
        SETTING(RefreshKingdomEnter);
        SETTING(TpHotkeys) = true;
        SETTING(DisableTpPuppet);
        SETTING(RefreshPurps);
        SETTING(InputDisplay);
        SETTING(DoorRefresh);
        SETTING(ShardRefresh);
        SETTING(FlowerPotRefresh);
        SETTING(NoCheckpointTouch);
        SETTING(SkipCloud);
        SETTING(AllCheckpoints);
        SETTING(Input2P);
        SETTING(HintPhotoSpawn);
        SETTING(LuigiPixels) = true;
        SETTING(SegmentTimer);
        SETTING(SegTimerSplit);
        SETTING(RefreshNpc);
        SETTING(DisableCoinStackSave);
        SETTING(PipeMazeOverride);
        InputDisplayColor mInputDisplayButtonColor = InputDisplayColor::White;
        InputDisplayColor mInputDisplayButtonPressedColor = InputDisplayColor::Pink;
        InputDisplayColor mInputDisplayStickColor = InputDisplayColor::White;
        InputDisplayColor mInputDisplayRingColor = InputDisplayColor::Gray;
        InputDisplayColor mInputDisplayBackColor = InputDisplayColor::Black;
        TpKey mSavePosKey = TpKey::DLeft;
        TpKey mLoadPosKey = TpKey::DRight;
        TpKey mRestartSceneKey = TpKey::DUp;
        SettingsHotkey mKillSceneKey = SettingsHotkey::Disabled;
        SettingsHotkey mHealMarioKey = SettingsHotkey::Disabled;
        SettingsHotkey mPrevSceneKey = SettingsHotkey::Disabled;
        SettingsHotkey mIncTpIndexKey = SettingsHotkey::Disabled;
        SettingsHotkey mDecTpIndexKey = SettingsHotkey::Disabled;
        SettingsHotkey mAddCoinsKey = SettingsHotkey::Disabled;
        SettingsHotkey mDecCoinsKey = SettingsHotkey::Disabled;
        SettingsHotkey mLifeUpKey = SettingsHotkey::Disabled;
        SettingsHotkey mIncPatternKey = SettingsHotkey::Disabled;
        SettingsHotkey mDecPatternKey = SettingsHotkey::Disabled;
        SettingsHotkey mTimerStartKey = SettingsHotkey::Disabled;
        SettingsHotkey mTimerEndKey = SettingsHotkey::Disabled;
        SettingsHotkey mTimerResetKey = SettingsHotkey::Disabled;
        SettingsHotkey mWarpLastCpKey = SettingsHotkey::Disabled;
        TimerHookType mTimerStartType = TimerHookType::None;
        TimerHookType mTimerEndType = TimerHookType::None;
        u8 mWorldTravelingStatus = 0;
        u8 mMenuCorner = 0;
        u8 mMenuKey = 0;
        u8 mMoonRefreshText = 0;
        u8 mWigglerPattern = 0;
        u8 mPipeRando1 = 0;
        u8 mPipeRando2 = 0;
        u8 mPipeRando3 = 0;
        ImVec2 mInfoPos = ImVec2(0, 0);
        ImVec2 mInputDisplayPosP2 = ImVec2(1600.f / 2, 900.f / 2);
        ImVec2 mTimerPos = ImVec2(1600.f / 2, 900.f / 2);
        ImVec2 mInputDisplayPos = ImVec2(1600.f / 2, 900.f / 2);
        // New settings go below this line so older settings files still load (see SaveFileHelper::loadSettings)
        SETTING(SkipBroodals);
        SETTING(AlwaysOdysseyPowerUp);
        SETTING(OdysseyInAllKingdoms);
        SETTING(NightMetroMultiMoon);
        // How many Start Type / End Type events are needed to start / stop the timer
        u8 mTimerStartCount = 1;
        u8 mTimerEndCount = 1;
        SETTING(SeedGrowing);
        // InputDisplayStyle (menu/InputDisplayStyles.h)
        u8 mInputDisplayStyle = 0;
    } mSettings;

    // Settings* getSettings() { return &mSettings; }

    static Settings* getSettings() { return sInstance ? &sInstance->mSettings : nullptr; }
};

} // namespace btt
