#include "menu/Menu.h"

#include "nn/oe.h"

#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"

#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/System/GameDataFunction.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "helpers/getHelper.h"
#include "helpers/InputHelper.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "menu/stage_warp.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::initCategories() {
    if (mCategoriesInitialized) return;
    if (!set) set = SettingsMgr::instance();
    if (!set || !set->getSettings()) return;

    mCategoriesInitialized = true;

    // 0: Options
    mCategories[0].name = "Options";
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableMoonRefresh, "Moon Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableGrayMoonRefresh, "Gray Moon Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshPurps, "Purple Coins Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDoorRefresh, "Doors Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableShardRefresh, "Moon Shards Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshKingdomEnter, "Kingdom Enter Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshWarpText, "Warp Text Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableFlowerPotRefresh, "Seeds Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshNpc, "Cutscene Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysManualCutscene, "Always Manual Cutscene"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysCheckpoints, "Always Checkpoints"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableAutoSave, "Disable Autosave"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableTpPuppet, "Disable TP Puppet"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSkipCloud, "Skip Cloud"));
    // Right column starts here
    mCategories[0].columnSplit = (int)mCategories[0].components.size();
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAllCheckpoints, "All Checkpoints"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableLuigiPixels, "Luigi Pixels"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableHintPhotoSpawn, "Spawn All Hint Art"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnablePipeMazeOverride, "Pipe Maze Override"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNoCheckpointTouch, "No Checkpoint Touch"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNoDamage, "No Damage"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableMusic, "Disable Music"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableMoonLock, "Disable Moon Lock"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSkipBroodals, "Skip Broodals"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysOdysseyPowerUp, "Always Odyssey Power Up"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableOdysseyInAllKingdoms, "Odyssey In All Kingdoms"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNightMetroMultiMoon, "Night Metro Multi Moon"));
    mCategories[0].components.push_back(new BoolMenuComponent(&mIsReloadPos, "Reload Scene at Pos"));

    // 1: Stages
    static int sSelectedStageIdx = 0;
    static int sSelectedScenario = 0;
    static const char* sStageNames[17] = {
        "Cap Kingdom", "Cascade Kingdom", "Sand Kingdom", "Wooded Kingdom",
        "Lake Kingdom", "Cloud Kingdom", "Lost Kingdom", "Metro Kingdom",
        "Snow Kingdom", "Seaside Kingdom", "Luncheon Kingdom", "Ruined Kingdom",
        "Bowser's Kingdom", "Moon Kingdom", "Mushroom Kingdom", "Dark Side", "Darker Side"
    };
    mCategories[1].name = "Stages";
    mCategories[1].components.push_back(new EnumMenuComponent<int>(&sSelectedStageIdx, sStageNames, 17, "Kingdom"));
    mCategories[1].components.push_back(new IntMenuComponent<int>(&sSelectedScenario, "Scenario", 0, 15));
    mCategories[1].components.push_back(new ButtonMenuComponent("Warp to Stage", []() {
        warpToKingdom(sSelectedStageIdx, sSelectedScenario);
    }));

    // 2: Misc
    mCategories[2].name = "Misc";
    mCategories[2].components.push_back(new EnumMenuComponent<int>(&mSelectedBody, bodyNames, 42, "Outfit Body"));
    mCategories[2].components.push_back(new EnumMenuComponent<int>(&mSelectedCap, capNames, 42, "Outfit Cap"));
    mCategories[2].components.push_back(new ButtonMenuComponent("Apply Outfit", [this]() {
        applyOutfit();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Save Position", [this]() {
        saveTeleport(tpStates[tpIndex]);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Load Position", [this]() {
        loadTeleport(tpStates[tpIndex]);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Restart Scene", [this]() {
        restartScene();
    }));
    mCategories[2].components.push_back(new BoolMenuComponent(&mIsReloadPos, "Reload at Pos"));
    mCategories[2].components.push_back(new ButtonMenuComponent("Restart at Pos", [this]() {
        triggerReloadPos();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Kill Mario", [this]() {
        if (playerHak) GameDataFunction::killPlayer(GameDataHolderWriter(playerHak));
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Damage Mario", [this]() {
        if (playerHak) {
            bool tmp = set->getSettings()->mIsEnableNoDamage;
            set->getSettings()->mIsEnableNoDamage = false;
            GameDataFunction::damagePlayer(GameDataHolderWriter(playerHak));
            set->getSettings()->mIsEnableNoDamage = tmp;
        }
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Life Up Heart", [this]() {
        if (playerHak) GameDataFunction::getLifeMaxUpItem(playerHak);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Heal Mario", [this]() {
        if (playerHak) GameDataFunction::recoveryPlayer(playerHak);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Add 1000 Coins", [this]() {
        if (stageScene) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), 1000);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Remove 1000 Coins", [this]() {
        if (stageScene) {
            int coinNum = GameDataFunction::getCoinNum(GameDataHolderAccessor(stageScene));
            if (coinNum < 1000) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -coinNum);
            else GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -1000);
        }
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Reload Scene", [this]() {
        if (stageScene) stageScene->kill();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Previous Scene", [this]() {
        if (stageScene) GameDataHolderAccessor(stageScene)->returnPrevStage();
    }));
    mCategories[2].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mWigglerPattern, WigglerPatterns, 23, "Wiggler Pattern"));

    // 3: Timer
    mCategories[3].name = "Timer";
    mCategories[3].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSegmentTimer, "Segment Timer"));
    mCategories[3].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSegTimerSplit, "Timer Split"));
    mCategories[3].components.push_back(new EnumMenuComponent<TimerHookType>(&set->getSettings()->mTimerStartType, TimerHookTypes, IM_ARRAYSIZE(TimerHookTypes), "Start Type"));
    mCategories[3].components.push_back(new EnumMenuComponent<TimerHookType>(&set->getSettings()->mTimerEndType, TimerHookTypes, IM_ARRAYSIZE(TimerHookTypes), "End Type"));
    mCategories[3].components.push_back(new ButtonMenuComponent("Start Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->start();
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Stop Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->stop();
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Reset Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->reset();
    }));
    static int sTimerPreset = 0;
    static const char* sTimerPresetNames[6] = { "Center", "Top Left", "Top Center", "Top Right", "Bottom Left", "Bottom Right" };
    static int sTimerX = (int)set->getSettings()->mTimerPos.x;
    static int sTimerY = (int)set->getSettings()->mTimerPos.y;
    mCategories[3].components.push_back(new EnumMenuComponent<int>(&sTimerPreset, sTimerPresetNames, 6, "Preset Pos", 34, [](int preset) {
        SettingsMgr* s = SettingsMgr::instance();
        if (!s || !s->getSettings()) return;
        constexpr static ImVec2 sPresets[6] = { { 800.f, 450.f }, { 40.f, 30.f },   { 700.f, 30.f },
                                                { 1380.f, 30.f }, { 40.f, 820.f }, { 1380.f, 820.f } };
        s->getSettings()->mTimerPos = sPresets[preset];
        sTimerX = (int)s->getSettings()->mTimerPos.x;
        sTimerY = (int)s->getSettings()->mTimerPos.y;
    }));
    mCategories[3].components.push_back(new IntMenuComponent<int>(&sTimerX, "Position X", 0, 1600, 25, 34, [](int x) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mTimerPos.x = (float)x;
    }));
    mCategories[3].components.push_back(new IntMenuComponent<int>(&sTimerY, "Position Y", 0, 900, 25, 34, [](int y) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mTimerPos.y = (float)y;
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Reset Position", []() {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) {
            s->getSettings()->mTimerPos = ImVec2(800.f, 450.f);
            sTimerPreset = 0;
            sTimerX = 800;
            sTimerY = 450;
        }
    }));

    // 4: Input Display
    static const char* sColorNames[13] = {
        "None", "White", "Gray", "Black", "Red", "Blue", "Yellow", "Green", "Orange", "Purple", "Pink", "LightBlue", "Multicolor"
    };
    static int sDisplayPreset = 0;
    static const char* sPresetNames[5] = { "Center", "Bottom Left", "Bottom Right", "Top Left", "Top Right" };
    static int sInputDisplayX = (int)set->getSettings()->mInputDisplayPos.x;
    static int sInputDisplayY = (int)set->getSettings()->mInputDisplayPos.y;

    mCategories[4].name = "Input Display";
    mCategories[4].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableInputDisplay, "Input Display"));
    mCategories[4].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableInput2P, "2P Mode"));
    mCategories[4].components.push_back(new EnumMenuComponent<int>(&sDisplayPreset, sPresetNames, 5, "Preset Pos", 34, [](int preset) {
        SettingsMgr* s = SettingsMgr::instance();
        if (!s || !s->getSettings()) return;
        switch (preset) {
            case 0: s->getSettings()->mInputDisplayPos = ImVec2(800.f, 450.f); break; // Center
            case 1: s->getSettings()->mInputDisplayPos = ImVec2(250.f, 720.f); break; // Bottom Left
            case 2: s->getSettings()->mInputDisplayPos = ImVec2(1380.f, 720.f); break; // Bottom Right
            case 3: s->getSettings()->mInputDisplayPos = ImVec2(250.f, 220.f); break; // Top Left
            case 4: s->getSettings()->mInputDisplayPos = ImVec2(1380.f, 220.f); break; // Top Right
        }
        sInputDisplayX = (int)s->getSettings()->mInputDisplayPos.x;
        sInputDisplayY = (int)s->getSettings()->mInputDisplayPos.y;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayX, "Position X", 0, 1600, 25, 34, [](int x) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPos.x = (float)x;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayY, "Position Y", 0, 900, 25, 34, [](int y) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPos.y = (float)y;
    }));
    mCategories[4].components.push_back(new ButtonMenuComponent("Reset Position", []() {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) {
            s->getSettings()->mInputDisplayPos = ImVec2(800.f, 450.f);
            sDisplayPreset = 0;
            sInputDisplayX = 800;
            sInputDisplayY = 450;
        }
    }));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayButtonColor, sColorNames, 13, "Button Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayButtonPressedColor, sColorNames, 13, "Pressed Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayStickColor, sColorNames, 13, "Stick Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayRingColor, sColorNames, 13, "Ring Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayBackColor, sColorNames, 13, "BG Color"));

    // 5: Keybinds
    mCategories[5].name = "Keybinds";
    mCategories[5].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mMenuKey, MenuKeys, 6, "Open Menu"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::TpKey>(&set->getSettings()->mSavePosKey, TpKeys, 20, "Save Position"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::TpKey>(&set->getSettings()->mLoadPosKey, TpKeys, 20, "Load Position"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::TpKey>(&set->getSettings()->mRestartSceneKey, TpKeys, 20, "Restart Scene"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mHealMarioKey, Keys, 11, "Heal Mario"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mLifeUpKey, Keys, 11, "Life Up"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mPrevSceneKey, Keys, 11, "Prev Scene"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mWarpLastCpKey, Keys, 11, "Warp Last CP"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mTimerStartKey, Keys, 11, "Timer Start"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mTimerEndKey, Keys, 11, "Timer End"));
    mCategories[5].components.push_back(new EnumMenuComponent<SettingsMgr::SettingsHotkey>(&set->getSettings()->mTimerResetKey, Keys, 11, "Timer Reset"));

    // 6: Info
    mCategories[6].name = "Info";
    mCategories[6].components.push_back(new BoolMenuComponent(&isEnableInfoWindow, "Info Window"));
    mCategories[6].components.push_back(new BoolMenuComponent(&isEnablePlayerInfo, "Player Info"));

    // 7: Settings
    mCategories[7].name = "Settings";
    mCategories[7].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mMenuCorner, Corners, 4, "Menu Corner"));
}

int Menu::getCategoryHeight(int id) {
    int height = 0;
    ImFont* font = ImGui::GetIO().Fonts->Fonts[0];
    for (int i = 0; i < id; i++) {
        ImVec2 textSize = font ? font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, mCategories[i].name) : ImVec2(100, (float)sCategoryFontSize);
        height += sCategorySpacing + (int)textSize.y + sCategorySpacing;
    }
    return height;
}

void Menu::draw() {
    if (!set) set = SettingsMgr::instance();
    initCategories();

    if (!mIsEnabledMenu) return;

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->Fonts[0];
    if (!font) return;

    mBgSize = ImVec2(0, 0);
    for (int i = 0; i < 8; i++) {
        ImVec2 textSize = font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, mCategories[i].name);
        float width = (float)sCategorySpacing * 2.0f + textSize.x + (float)sCategorySpacing * 2.0f;
        mBgSize.x = std::max(mBgSize.x, width);
    }
    mBgSize.y = (float)getCategoryHeight(8);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // 1. Sidebar background: translucent dark bar
    drawList->AddRectFilled(ImVec2(0, 0), mBgSize, IM_COL32(0, 0, 0, 140));

    // 2. Highlight for selected category
    drawList->AddRectFilled(
        ImVec2(0, (float)getCategoryHeight(mCurrentCategory)),
        ImVec2(mBgSize.x, (float)getCategoryHeight(mCurrentCategory + 1)),
        IM_COL32(255, 255, 255, 76)
    );

    // 3. Category labels in white
    float curY = 0;
    for (int i = 0; i < 8; i++) {
        ImVec2 textSize = font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, mCategories[i].name);
        curY += (float)sCategorySpacing;
        drawList->AddText(font, (float)sCategoryFontSize, ImVec2((float)sCategorySpacing * 2.0f, curY), IM_COL32(255, 255, 255, 255), mCategories[i].name);
        curY += textSize.y + (float)sCategorySpacing;
    }

    // 4. Expanded drawer for current category items
    if (mIsExpandedCurrentCategory) {
        drawExpandedCategory();
    }
}

void Menu::drawExpandedCategory() {
    auto& cat = mCategories[mCurrentCategory];
    if (cat.components.empty()) return;

    if (mCurrentComponentInCategory >= (int)cat.components.size())
        mCurrentComponentInCategory = (int)cat.components.size() - 1;
    if (mCurrentComponentInCategory < 0)
        mCurrentComponentInCategory = 0;

    if (cat.columnSplit > 0 && cat.columnSplit < (int)cat.components.size()) {
        drawTwoColumnCategory(cat);
        return;
    }

    float subX = 0;
    for (int i = 0; i < (int)cat.components.size(); i++) {
        MenuComponent* comp = cat.components[i];
        if (!comp) continue;
        ImVec2 size = comp->getSize();
        subX = std::max(subX, size.x);
    }

    int maxVisible = 14;
    int scrollOffset = 0;
    if ((int)cat.components.size() > maxVisible) {
        if (mCurrentComponentInCategory >= maxVisible - 2) {
            scrollOffset = mCurrentComponentInCategory - maxVisible + 3;
        }
        if (scrollOffset + maxVisible > (int)cat.components.size()) {
            scrollOffset = (int)cat.components.size() - maxVisible;
        }
        if (scrollOffset < 0) scrollOffset = 0;
    }
    int endIndex = std::min((int)cat.components.size(), scrollOffset + maxVisible);

    float startY = (float)getCategoryHeight(mCurrentCategory);
    float totalDrawHeight = (endIndex - scrollOffset) * 44.0f;
    if (startY + totalDrawHeight > 700.0f) {
        startY = std::max(10.0f, 700.0f - totalDrawHeight);
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    float drawY = 0;
    for (int i = scrollOffset; i < endIndex; i++) {
        MenuComponent* comp = cat.components[i];
        if (!comp) continue;
        ImVec2 size = comp->getSize();
        float itemH = size.y > 0 ? size.y : 40.0f;

        // Card background (translucent dark rounded rectangle)
        drawList->AddRectFilled(
            ImVec2(mBgSize.x + 8.0f, startY + drawY),
            ImVec2(mBgSize.x + 8.0f + subX, startY + drawY + itemH),
            IM_COL32(0, 0, 0, 150),
            12.0f
        );

        // Highlight for selected component
        if (mCurrentComponentInCategory == i) {
            ImU32 highlightCol = mIsFocusedOnCurrentComponent ? IM_COL32(255, 60, 60, 100) : IM_COL32(255, 255, 255, 76);
            drawList->AddRectFilled(
                ImVec2(mBgSize.x + 8.0f, startY + drawY),
                ImVec2(mBgSize.x + 8.0f + subX, startY + drawY + itemH),
                highlightCol,
                12.0f
            );
        }

        comp->setIsHovered(mCurrentComponentInCategory == i);
        comp->setIsFocused(mCurrentComponentInCategory == i && mIsFocusedOnCurrentComponent);
        comp->draw(ImVec2(mBgSize.x + 8.0f, startY + drawY));

        drawY += itemH + 3.0f;
    }
}

void Menu::drawTwoColumnCategory(Category& cat) {
    const int count = (int)cat.components.size();
    const int split = cat.columnSplit;
    const int ranges[2][2] = {{0, split}, {split, count}};

    float colWidth[2] = {0, 0};
    for (int c = 0; c < 2; c++) {
        for (int i = ranges[c][0]; i < ranges[c][1]; i++) {
            if (cat.components[i]) colWidth[c] = std::max(colWidth[c], cat.components[i]->getSize().x);
        }
    }

    int maxRows = std::max(split, count - split);
    float startY = (float)getCategoryHeight(mCurrentCategory);
    float totalDrawHeight = maxRows * 44.0f;
    if (startY + totalDrawHeight > 700.0f) {
        startY = std::max(10.0f, 700.0f - totalDrawHeight);
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    float colX = mBgSize.x + 8.0f;
    for (int c = 0; c < 2; c++) {
        float drawY = 0;
        for (int i = ranges[c][0]; i < ranges[c][1]; i++) {
            MenuComponent* comp = cat.components[i];
            if (!comp) continue;
            ImVec2 size = comp->getSize();
            float itemH = size.y > 0 ? size.y : 40.0f;
            ImVec2 tl(colX, startY + drawY);
            ImVec2 br(colX + colWidth[c], startY + drawY + itemH);

            drawList->AddRectFilled(tl, br, IM_COL32(0, 0, 0, 150), 12.0f);
            if (mCurrentComponentInCategory == i) {
                ImU32 highlightCol = mIsFocusedOnCurrentComponent ? IM_COL32(255, 60, 60, 100) : IM_COL32(255, 255, 255, 76);
                drawList->AddRectFilled(tl, br, highlightCol, 12.0f);
            }

            comp->setIsHovered(mCurrentComponentInCategory == i);
            comp->setIsFocused(mCurrentComponentInCategory == i && mIsFocusedOnCurrentComponent);
            comp->draw(tl);

            drawY += itemH + 3.0f;
        }
        colX += colWidth[c] + 8.0f;
    }
}

static bool isPressMenuKey() {
    if (InputHelper::isHoldL() && InputHelper::isPressPadLeft()) return true;
    switch (SettingsMgr::getSettings()->mMenuKey) {
    case 0: return InputHelper::isPressStickL();
    case 1: return InputHelper::isPressStickR();
    case 2: return InputHelper::isHoldL() && InputHelper::isPressPadLeft();
    case 3: return InputHelper::isHoldL() && InputHelper::isPressPadRight();
    case 4: return InputHelper::isHoldR() && InputHelper::isPressPadLeft();
    case 5: return InputHelper::isHoldR() && InputHelper::isPressPadRight();
    default: return false;
    }
}

void Menu::updateMenuInput() {
    if (isPressMenuKey()) {
        if (mIsEnabledMenu) hideMenu();
        else showMenu();
        return;
    }

    if (!mIsEnabledMenu) return;

    auto& cat = mCategories[mCurrentCategory];

    if (!mIsExpandedCurrentCategory) {
        if (InputHelper::isPressPadDown()) {
            mCurrentCategory = (mCurrentCategory + 1) % 8;
        } else if (InputHelper::isPressPadUp()) {
            mCurrentCategory = (mCurrentCategory + 7) % 8;
        }

        if (InputHelper::isPressPadRight() || InputHelper::isPressA()) {
            mIsExpandedCurrentCategory = true;
            mCurrentComponentInCategory = 0;
            mIsFocusedOnCurrentComponent = false;
        }
    } else {
        const int count = (int)cat.components.size();
        const bool twoColumns = cat.columnSplit > 0 && cat.columnSplit < count;
        const bool inRightColumn = twoColumns && mCurrentComponentInCategory >= cat.columnSplit;

        if (!mIsFocusedOnCurrentComponent && twoColumns && (InputHelper::isPressPadDown() || InputHelper::isPressPadUp() ||
                                                            InputHelper::isPressPadRight() || (inRightColumn && InputHelper::isPressPadLeft()))) {
            // Two-column navigation: up/down wrap inside the current column, left/right switch column on the same row
            const int colStart = inRightColumn ? cat.columnSplit : 0;
            const int colSize = inRightColumn ? count - cat.columnSplit : cat.columnSplit;
            const int row = mCurrentComponentInCategory - colStart;
            if (InputHelper::isPressPadDown()) {
                mCurrentComponentInCategory = colStart + (row + 1) % colSize;
            } else if (InputHelper::isPressPadUp()) {
                mCurrentComponentInCategory = colStart + (row + colSize - 1) % colSize;
            } else if (InputHelper::isPressPadRight()) {
                if (!inRightColumn)
                    mCurrentComponentInCategory = cat.columnSplit + std::min(row, count - cat.columnSplit - 1);
            } else {
                mCurrentComponentInCategory = std::min(row, cat.columnSplit - 1);
            }
        } else if (!mIsFocusedOnCurrentComponent) {
            if (InputHelper::isPressPadDown()) {
                if (!cat.components.empty()) {
                    mCurrentComponentInCategory = (mCurrentComponentInCategory + 1) % cat.components.size();
                }
            } else if (InputHelper::isPressPadUp()) {
                if (!cat.components.empty()) {
                    mCurrentComponentInCategory = (mCurrentComponentInCategory + (int)cat.components.size() - 1) % cat.components.size();
                }
            } else if (InputHelper::isPressPadLeft() || InputHelper::isPressB()) {
                mIsExpandedCurrentCategory = false;
            } else if (InputHelper::isPressA()) {
                if (mCurrentComponentInCategory >= 0 && mCurrentComponentInCategory < (int)cat.components.size()) {
                    auto* comp = cat.components[mCurrentComponentInCategory];
                    if (comp->canHaveFocus()) {
                        mIsFocusedOnCurrentComponent = true;
                    } else {
                        comp->onAction();
                    }
                }
            }
        } else {
            if (mCurrentComponentInCategory >= 0 && mCurrentComponentInCategory < (int)cat.components.size()) {
                auto* comp = cat.components[mCurrentComponentInCategory];
                if (InputHelper::isPressPadLeft()) {
                    comp->onLeft();
                } else if (InputHelper::isPressPadRight()) {
                    comp->onRight();
                } else if (InputHelper::isPressA() || InputHelper::isPressB()) {
                    mIsFocusedOnCurrentComponent = false;
                }
            }
        }
    }
}

void Menu::handleAlways() {
    set = SettingsMgr::instance();
    gameSeq = helpers::tryGetHakoniwaSequence();
    stageScene = helpers::tryGetStageScene(gameSeq);
    if (stageScene) {
        if (strcmp(
                GameDataFunction::getCurrentStageName(gameSeq->mGameDataHolderAccessor), "ClashWorldHomeStage"
            ) == 0)
            noGetPlayer = false;
    }
    if (noGetPlayer) {
        playerHak = nullptr;
    } else {
        playerHak = helpers::tryGetPlayerActorHakoniwa(gameSeq);
    }

    menuTimer++;

    updateMenuInput();

    handleHotkeys();

    if (mCancelCameraInterpoleFrames > 0) {
        mCancelCameraInterpoleFrames--;
        if (playerHak) {
            al::LiveActor* hack = playerHak->mHackKeeper ? playerHak->mHackKeeper->mHackActor : nullptr;
            al::LiveActor* target = hack ? hack : (al::LiveActor*)playerHak;
            al::requestCancelCameraInterpole(target, 0);
        }
        if (stageScene) {
            al::requestCancelCameraInterpole(stageScene, 0);
        }
    }

    if (mReloadState == 1) {
        // Step 1: Wait for old scene to die or become invalid
        if (!stageScene || stageScene != mReloadOldScene || !stageScene->mIsAlive) {
            mReloadState = 2;
            reloadPosTimer = 0;
        }
    } else if (mReloadState == 2) {
        // Step 2: New scene has loaded, wait for player to initialize
        if (stageScene && stageScene != mReloadOldScene && stageScene->mIsAlive && playerHak) {
            reloadPosTimer++;
            if (reloadPosTimer >= 10) {
                playerHak->startDemoPuppetable();
                al::setTrans(playerHak, reloadStagePos);
                al::updatePoseQuat(playerHak, reloadStageQuat);
                al::setVelocityZero(playerHak);
                playerHak->endDemoPuppetable();
                al::requestCancelCameraInterpole(playerHak, 0);
                if (stageScene) al::requestCancelCameraInterpole(stageScene, 0);
                mCancelCameraInterpoleFrames = 10;
                menuTimer = 0;
                mReloadState = 0;
                reloadPosTimer = -1;
                mReloadOldScene = nullptr;
                setPopupText("Reloaded at Position");
            }
        }
    } else if (reloadPosTimer != -1 && playerHak) {
        // Safety fallback
        reloadPosTimer++;
        if (reloadPosTimer >= 15) {
            playerHak->startDemoPuppetable();
            al::setTrans(playerHak, reloadStagePos);
            al::updatePoseQuat(playerHak, reloadStageQuat);
            al::setVelocityZero(playerHak);
            playerHak->endDemoPuppetable();
            al::requestCancelCameraInterpole(playerHak, 0);
            menuTimer = 0;
            reloadPosTimer = -1;
            setPopupText("Reloaded at Position");
        }
    }
}

void Menu::hideMenu() {
    mIsEnabledMenu = false;
    mIsExpandedCurrentCategory = false;
    mIsFocusedOnCurrentComponent = false;
    prevMouseDis = InputHelper::isDisableMouse();
    InputHelper::setDisableMouse(true);
}

void Menu::showMenu() {
    mIsEnabledMenu = true;
    mIsExpandedCurrentCategory = false;
    mIsFocusedOnCurrentComponent = false;
    menuTimer = 0;
    InputHelper::setDisableMouse(prevMouseDis);
}

void Menu::drawPopup() {
    if (!mIsPopup) return;
    if (++popupTimer > 120) {
        mIsPopup = false;
        popupTimer = 0;
        return;
    }
    ImGui::Begin(
        "Popup", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoNavFocus
    );
    ImGui::SetWindowSize(ImVec2(0, 0));
    ImGui::SetWindowPos(ImVec2(mWindowSize.x + 10, 0));
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("%s", popupText);
    ImGui::End();
}

const char* Menu::getMoonRefreshText() {
    if (strcmp(MoonRefreshTexts[set->getSettings()->mMoonRefreshText], "<blank>") != 0) {
        return MoonRefreshTexts[set->getSettings()->mMoonRefreshText];
    } else {
        return "";
    }
}

ImVec2 Menu::getCornerPos(int corner) {
    ImVec2 pos = ImVec2(0, 0);
    nn::oe::OperationMode mode = nn::oe::GetOperationMode();
    if (mode == nn::oe::OperationMode_Docked) {
        switch (corner) {
        case 0: pos = ImVec2(0, 0); break;
        case 1: pos = ImVec2(1600 - mWindowSize.x, 0); break;
        case 2: pos = ImVec2(0, 900 - mWindowSize.y); break;
        case 3: pos = ImVec2(1600 - mWindowSize.x, 900 - mWindowSize.y); break;
        }
    } else {
        switch (corner) {
        case 0: pos = ImVec2(0, 0); break;
        case 1: pos = ImVec2(1280 - mWindowSize.x, 0); break;
        case 2: pos = ImVec2(0, 720 - mWindowSize.y); break;
        case 3: pos = ImVec2(1280 - mWindowSize.x, 720 - mWindowSize.y); break;
        }
    }
    return pos;
}

void Menu::applyOutfit() {
    GameDataHolder* holder = nullptr;
    if (stageScene) {
        holder = GameDataHolderAccessor(stageScene).mData;
    } else if (gameSeq) {
        holder = gameSeq->mGameDataHolderAccessor.mData;
    }

    if (holder) {
        GameDataHolderWriter writer(holder);
        if (mSelectedCap >= 0 && mSelectedCap < 42) {
            GameDataFunction::wearCap(writer, capNames[mSelectedCap]);
        }
        if (mSelectedBody >= 0 && mSelectedBody < 42) {
            GameDataFunction::wearCostume(writer, bodyNames[mSelectedBody]);
        }
        if (stageScene) {
            stageScene->kill();
        }
        setPopupText("Outfit Applied");
    }
}

} // namespace btt
