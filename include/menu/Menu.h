#pragma once

#include "sead/heap/seadDisposer.h"
#include "sead/math/seadQuat.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"

#include <cstdio>

#include "imgui.h"
#include "src/settings/SettingsMgr.h"
#include "menu/MenuComponent.h"
#include <vector>

class WorldTravelingNpc;

namespace btt {

class Menu {
    SEAD_SINGLETON_DISPOSER(Menu)

public:
    constexpr static int sCategorySpacing = 6;
    constexpr static int sCategoryFontSize = 38;

    struct Category {
        const char* name;
        std::vector<MenuComponent*> components;
        int columnSplit = 0; // > 0: components from this index are drawn in a second column on the right
    };

    Category mCategories[8];
    bool mCategoriesInitialized = false;

    void initCategories();
    void drawExpandedCategory();
    void drawTwoColumnCategory(Category& cat);
    int getCategoryHeight(int id);
    void updateMenuInput();

    int mCurrentCategory = 0;
    int mCurrentComponentInCategory = 0;
    bool mIsExpandedCurrentCategory = false;
    bool mIsFocusedOnCurrentComponent = false;
    ImVec2 mBgSize = ImVec2(0, 0);

    Menu() = default;
    void draw();
    void handleAlways();
    void drawInfoWindow();
    void drawPopup();

    void setLatestMapTarget(void* target) { mLastMapTarget = target; }

    bool isPatternReverse();
    int getPatternTarget(int a);

    const char* getMoonRefreshText();
    void hideMenu();
    void showMenu();
    void restartScene();
    void triggerReloadPos();
    void applyOutfit();

public:
    struct TpState {
        bool saved;
        sead::Vector3f pos = { 0, 0, 0 };
        sead::Quatf quat = { 0, 0, 0, 0 };
        char stageName[0x40] = {};
    };

    TpState tpStates[0x8];

    bool mIsEnableNoclip = false;
    bool noGetPlayer = false;
    bool mIsEnabledMenu = false;
    WorldTravelingNpc* mWorldTravelingNpc = nullptr;
    const char* mWorldTravelingStatus = nullptr;

    void setPopupText(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(popupText, sizeof(popupText), fmt, args);
        va_end(args);
        mIsPopup = true;
        popupTimer = 0;
    }

private:
    void drawPageOptions();
    void drawPageInputDisplay();
    void drawPageHotkeys();
    void drawPageInfo();
    void drawPageMisc();
    void drawTeleportCat();
    void drawTimerCat();

    void saveTeleport(TpState& state);
    void loadTeleport(TpState& state);
    void handleHotkeys();
    void handlePopup();

    bool isHotkey(SettingsMgr::SettingsHotkey key);
    bool isTpKey(SettingsMgr::TpKey key);

    ImVec2 getCornerPos(int corner);

    void drawComplexHeapTreeItem(sead::Heap* heap);

private:
    ImVec2 mWindowSize = ImVec2(500, 400);
    ImGuiID prevNavId = 0;
    u64 menuTimer = 0;
    u64 popupTimer = 0;

    HakoniwaSequence* gameSeq;
    StageScene* stageScene;
    PlayerActorHakoniwa* playerHak;
    SettingsMgr* set;

    void* mLastMapTarget = nullptr;

    bool mIsPopup = false;
    bool prevMouseDis = true;
    bool wasMenuDisabled = false;
    bool isEnableInfoWindow = false;
    bool isEnablePlayerInfo = false;
    bool mIsReloadPos = false;
    int reloadPosTimer = -1;
    StageScene* mReloadOldScene = nullptr;
    int mReloadState = 0;
    int mCancelCameraInterpoleFrames = 0;
    int mSelectedBody = 0;
    int mSelectedCap = 0;
    int tpIndex = 0;
    sead::Vector3f reloadStagePos = { 0, 0, 0 };
    sead::Quatf reloadStageQuat = { 0, 0, 0, 0 };

    char popupText[0x40] = "Input Disabled";

    constexpr static const char* Corners[4] { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
    constexpr static const char* MoonRefreshTexts[] {
        "BTT Studio", "BTT", "Hi BTT", "I am Cool", "Super Mario Odyssey", "Entrance to Shiveria", "<blank>"
    };
    constexpr static const char* WigglerPatterns[23] { "Random",  "Ghost",    "Nose",  "C",    "W",
                                                       "J",       "Medal",    "Plane", "5",    "Hangman",
                                                       "Spanish", "Siblings", "Snake", "8",    "Mushroom",
                                                       "Z",       "Tetris",   "Ear",   "Bomb", "Bird",
                                                       "L",       "O",        "Star" };
    constexpr static const char* worldTravelingStatus[] = { "<Normal>", "First Talk", "Metro",
                                                            "Cascade",  "Luncheon",   "Moon",
                                                            "Mushroom", "Last Talk",  "After Last Talk" };

    constexpr static const char* Keys[] {
        "Disabled", "None", "L", "R", "ZL", "ZR", "L + R", "L + ZL", "L + ZR", "R + ZL", "R + ZR",
    };
    constexpr static const char* MenuKeys[] { "LStick",     "RStick",    "L + DLeft",
                                              "L + DRight", "R + DLeft", "R + DRight" };
    constexpr static const char* TpKeys[] {
        "D-Pad Left",
        "D-Pad Right",
        "D-Pad Down",
        "D-Pad Up",
        "L + D-Pad Left",
        "L + D-Pad Right",
        "L + D-Pad Down",
        "L + D-Pad Up",
        "ZL + D-Pad Left",
        "ZL + D-Pad Right",
        "ZL + D-Pad Down",
        "ZL + D-Pad Up",
        "R + D-Pad Left",
        "R + D-Pad Right",
        "LStick",
        "RStick",
        "L + LStick",
        "L + RStick",
        "ZL + ZR",
        "Disabled",
    };

    constexpr static const char* bodyNames[] = {
        "Mario",          "Mario64",         "Mario64Metal",      "MarioAloha",
        "MarioArmor",     "MarioBone",       "MarioClown",        "MarioColorClassic",
        "MarioColorGold", "MarioColorLuigi", "MarioColorWaluigi", "MarioColorWario",
        "MarioCook",      "MarioDiddyKong",  "MarioDoctor",       "MarioExplorer",
        "MarioFootball",  "MarioGolf",       "MarioGunman",       "MarioHakama",
        "MarioHappi",     "MarioKing",       "MarioKoopa",        "MarioMaker",
        "MarioMechanic",  "MarioNew3DS",     "MarioPainter",      "MarioPeach",
        "MarioPilot",     "MarioPirate",     "MarioPoncho",       "MarioPrimitiveMan",
        "MarioSailor",    "MarioScientist",  "MarioShopman",      "MarioSnowSuit",
        "MarioSpaceSuit", "MarioSuit",       "MarioSwimwear",     "MarioTailCoat",
        "MarioTuxedo",    "MarioUnderwear"
    };

    constexpr static const char* capNames[] = {
        "Mario",          "Mario64",         "Mario64Metal",      "MarioAloha",
        "MarioArmor",     "MarioBone",       "MarioClown",        "MarioColorClassic",
        "MarioColorGold", "MarioColorLuigi", "MarioColorWaluigi", "MarioColorWario",
        "MarioCook",      "MarioDiddyKong",  "MarioDoctor",       "MarioExplorer",
        "MarioFootball",  "MarioGolf",       "MarioGunman",       "MarioHakama",
        "MarioHappi",     "MarioKing",       "MarioKoopa",        "MarioMaker",
        "MarioMechanic",  "MarioNew3DS",     "MarioPainter",      "MarioPeach",
        "MarioPilot",     "MarioPirate",     "MarioPoncho",       "MarioPrimitiveMan",
        "MarioSailor",    "MarioScientist",  "MarioShopman",      "MarioSnowSuit",
        "MarioSpaceSuit", "MarioSuit",       "MarioSwimwear",     "MarioTailCoat",
        "MarioTuxedo",    "MarioInvisible"
    };

    // Timer types offered in the menus, without the unimplemented one (Capture Glow)
    constexpr static TimerHookType VisibleTimerHookTypes[] = {
        TimerHookType::None,         TimerHookType::GameStart,   TimerHookType::OdysseyPowerUp,
        TimerHookType::BlackScreen,  TimerHookType::WhiteScreen, TimerHookType::Capture,
        TimerHookType::Damage,       TimerHookType::EscapeWire,  TimerHookType::MoonGet,
        TimerHookType::ShineTick,    TimerHookType::TwoMoons,    TimerHookType::TwoMoonsTick,
        TimerHookType::ShineGrab,
    };
    constexpr static const char* VisibleTimerHookTypeNames[] = {
        "None",         "Press Start", "Odyssey Power Up", "Black Screen",   "White Screen",   "Capture",
        "Damage",       "Escape Wire", "Moon Get", "Shine Tick", "2 Moons (Grab)", "2 Moons (Tick)", "Shine Grab",
    };
    static int findVisibleTimerHookType(TimerHookType type) {
        for (int i = 0; i < (int)(sizeof(VisibleTimerHookTypes) / sizeof(VisibleTimerHookTypes[0])); i++)
            if (VisibleTimerHookTypes[i] == type) return i;
        return 0;
    }
};
} // namespace btt
