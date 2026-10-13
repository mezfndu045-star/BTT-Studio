#include "hk/sail/detail.h"

#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/Nerve.h"
#include "al/Library/Nerve/NerveKeeper.h"
#include "al/Library/Nerve/NerveStateCtrl.h"

#include "custom/game/Scene/StageSceneStateStageMap.h"
#include "game/MapObj/ChangeStageInfo.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"

#include <cxxabi.h>
#include <typeinfo>

#include "helpers/getHelper.h"
#include "helpers/saveFileHelper.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "menu/Menu.h"
#include "menu/stage_warp.h"
#include "menu/Timer.h"

using namespace btt;

void Menu::drawTeleportCat() {
    if (ImGui::CollapsingHeader("Teleport")) {
        ImGui::Indent();
        ImGui::PushItemWidth(200);
        ImGui::InputInt("Teleport Index", &tpIndex);
        if (tpIndex < 0) tpIndex = hk::util::arraySize(tpStates) - 1;
        if (tpIndex >= hk::util::arraySize(tpStates)) tpIndex = 0;
        ImGui::PopItemWidth();

        if (ImGui::Button("Save")) saveTeleport(tpStates[tpIndex]);
        ImGui::SameLine();
        if (ImGui::Button("Load")) loadTeleport(tpStates[tpIndex]);
        ImGui::SameLine();
        ImGui::PushID("TpHotkeys");
        ImGui::Checkbox("Hotkeys", &set->getSettings()->mIsEnableTpHotkeys);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Checkbox("Saved", &tpStates[tpIndex].saved);
        ImGui::Text("Stage: %s", tpStates[tpIndex].stageName);
        ImGui::EndDisabled();

        if (ImGui::Button("Save To File")) {
            SaveFileHelper::instance()->saveTeleport(tpStates, hk::util::arraySize(tpStates));
        }

        ImGui::SameLine();
        if (ImGui::Button("Load From File")) {
            SaveFileHelper::instance()->loadTeleport(
                tpStates, hk::util::arraySize(tpStates), al::getStationedHeap()
            );
        }
        ImGui::Unindent();
    }
}

void Menu::saveTeleport(TpState& state) {
    if (!stageScene || !playerHak) return;

    al::LiveActor* hack = playerHak->mHackKeeper ? playerHak->mHackKeeper->mHackActor : nullptr;
    al::LiveActor* target = hack ? hack : (al::LiveActor*)playerHak;

    state.saved = true;
    state.pos = al::getTrans(target);
    state.quat = al::getQuat(target);
    strcpy(
        state.stageName,
        getEnglishName(GameDataFunction::getCurrentStageName(GameDataHolderAccessor(stageScene)))
    );
    setPopupText("Position Saved");
}

void Menu::loadTeleport(TpState& state) {
    if (!stageScene || !playerHak) return;
    if (!state.saved) {
        setPopupText("No Saved Position");
        return;
    }
    al::LiveActor* hack = playerHak->mHackKeeper ? playerHak->mHackKeeper->mHackActor : nullptr;

    if (hack) {
        al::offCollide(hack);
        al::setTrans(hack, state.pos);
        al::updatePoseQuat(hack, state.quat);
        al::setVelocityZero(hack);
        al::onCollide(hack);
        al::requestCancelCameraInterpole(hack, 0);
        if (stageScene) al::requestCancelCameraInterpole(stageScene, 0);
        mCancelCameraInterpoleFrames = 10;
        menuTimer = 0;
        setPopupText("Position Loaded");
        return;
    }

    if (set->getSettings()->mIsEnableDisableTpPuppet && helpers::isGetShineState(stageScene)) {
        al::setTrans(playerHak, state.pos);
        al::updatePoseQuat(playerHak, state.quat);
        al::setVelocityZero(playerHak);
        menuTimer = 0;
    } else {
        playerHak->startDemoPuppetable();
        al::setTrans(playerHak, state.pos);
        al::updatePoseQuat(playerHak, state.quat);
        al::setVelocityZero(playerHak);
        playerHak->endDemoPuppetable();
        menuTimer = 0;
    }
    al::requestCancelCameraInterpole(playerHak, 0);
    if (stageScene) al::requestCancelCameraInterpole(stageScene, 0);
    mCancelCameraInterpoleFrames = 10;
    setPopupText("Position Loaded");
}

void Menu::drawTimerCat() {
    if (ImGui::CollapsingHeader("Segment Timer")) {
        ImGui::Indent();
        ImGui::Checkbox("Enable", &set->getSettings()->mIsEnableSegmentTimer);
        ImGui::SameLine();
        if (ImGui::Button("Start")) {
            btt::Timer::sInstance->start();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            btt::Timer::sInstance->stop();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            btt::Timer::sInstance->reset();
        }
        static int startType = (u8)set->getSettings()->mTimerStartType;
        static int endType = (u8)set->getSettings()->mTimerEndType;

        if (ImGui::Combo("Start Type", &startType, TimerHookTypes, IM_ARRAYSIZE(TimerHookTypes)))
            set->getSettings()->mTimerStartType = (TimerHookType)startType;
        if (ImGui::Combo("End Type", &endType, TimerHookTypes, IM_ARRAYSIZE(TimerHookTypes)))
            set->getSettings()->mTimerEndType = (TimerHookType)endType;
        static int posX = set->getSettings()->mTimerPos.x;
        static int posY = set->getSettings()->mTimerPos.y;
        ImGui::PushItemWidth(200);
        if (ImGui::InputInt("Timer X", &posX, 5)) set->getSettings()->mTimerPos.x = posX;
        if (ImGui::InputInt("Timer Y", &posY, 5)) set->getSettings()->mTimerPos.y = posY;
        ImGui::PopItemWidth();
        ImGui::Unindent();
    }
}

void Menu::drawPageMisc() {
    if (ImGui::CollapsingHeader("Misc##page")) {
        ImGui::Indent();
        drawTeleportCat();
        if (ImGui::CollapsingHeader("Outfit")) {
            ImGui::Indent();
            ImGui::Combo("Body", &mSelectedBody, bodyNames, IM_ARRAYSIZE(bodyNames));
            ImGui::Combo("Cap", &mSelectedCap, capNames, IM_ARRAYSIZE(capNames));
            if (ImGui::Button("Apply Outfit")) {
                applyOutfit();
            }
            ImGui::Unindent();
        }
        drawTimerCat();
        if (ImGui::CollapsingHeader("Pipe Randomizer Override")) {
            static int pipe1 = set->getSettings()->mPipeRando1;
            static int pipe2 = set->getSettings()->mPipeRando2;
            static int pipe3 = set->getSettings()->mPipeRando3;
            ImGui::Checkbox("Enabled", &set->getSettings()->mIsEnablePipeMazeOverride);

            if (ImGui::InputInt("2 Pipes", &pipe1)) {
                if (pipe1 < 0) pipe1 = 1;
                if (pipe1 > 1) pipe1 = 0;
                set->getSettings()->mPipeRando1 = pipe1;
            }
            if (ImGui::InputInt("3 Pipes", &pipe2)) {
                if (pipe2 < 0) pipe2 = 2;
                if (pipe2 > 2) pipe2 = 0;
                set->getSettings()->mPipeRando2 = pipe2;
            }
            if (ImGui::InputInt("5 Pipes", &pipe3)) {
                if (pipe3 < 0) pipe3 = 4;
                if (pipe3 > 4) pipe3 = 0;
                set->getSettings()->mPipeRando3 = pipe3;
            }
        }
        if (GImGui->NavId == ImGui::GetID("Pipe Randomizer Override")) {
            ImGui::SetTooltip("For changes to take effect \nreload the scene");
        }

        ImGui::PushItemWidth(200);
        static int pattern = set->getSettings()->mWigglerPattern;
        static int status = set->getSettings()->mWorldTravelingStatus;
        if (ImGui::Combo("Wiggler Pattern", &pattern, WigglerPatterns, IM_ARRAYSIZE(WigglerPatterns)))
            set->getSettings()->mWigglerPattern = pattern;
        if (ImGui::Combo("Tourist Status", &status, worldTravelingStatus, IM_ARRAYSIZE(worldTravelingStatus)))
            set->getSettings()->mWorldTravelingStatus = status;
        ImGui::PopItemWidth();

        if (ImGui::Button("Kill Mario")) {
            if (playerHak) GameDataFunction::killPlayer(GameDataHolderWriter(playerHak));
        }
        ImGui::SameLine();
        if (ImGui::Button("Damage Mario")) {
            if (playerHak) {
                bool tmpDamage = SettingsMgr::instance()->getSettings()->mIsEnableNoDamage;
                SettingsMgr::instance()->getSettings()->mIsEnableNoDamage = false;
                GameDataFunction::damagePlayer(GameDataHolderWriter(playerHak));
                SettingsMgr::instance()->getSettings()->mIsEnableNoDamage = tmpDamage;
            }
        }
        if (ImGui::Button("Life Up Heart")) {
            if (playerHak) GameDataFunction::getLifeMaxUpItem(playerHak);
        }
        ImGui::SameLine();
        if (ImGui::Button("Heal Mario")) {
            if (playerHak) GameDataFunction::recoveryPlayer(playerHak);
        }
        if (ImGui::Button("Add 1000 coins")) {
            if (stageScene) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), 1000);
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove 1000 coins")) {
            if (stageScene) {
                int coinNum = GameDataFunction::getCoinNum(GameDataHolderAccessor(stageScene));
                if (coinNum < 1000)
                    GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -coinNum);
                else
                    GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -1000);
            }
        }
        // if (ImGui::Button("Remove Cappy")) {
        //     if (playerHak) GameDataFunction::disableCapByPlacement((al::LiveActor*)playerHak->mHackCap);
        // }
        if (ImGui::Button("Restart Scene")) {
            restartScene();
        }
        ImGui::SameLine();
        if (ImGui::Button("Restart at Pos")) {
            triggerReloadPos();
        }
        ImGui::SameLine();
        if (ImGui::Button("Previous Scene")) {
            if (stageScene) GameDataHolderAccessor(stageScene)->returnPrevStage();
        }
        if (ImGui::Button("Warp to Last Checkpoint")) {
            if (stageScene) {
                if (mLastMapTarget) {
                    stageScene->mStateCollectionList->mStateStageMap->mMapThing = mLastMapTarget;
                    ptr addr = hk::sail::lookupSymbolFromDb<>("StageSceneNrvWarpToCheckpoint");
                    stageScene->getNerveKeeper()->setNerve((al::Nerve*)addr);
                }
            }
        }
        ImGui::Checkbox("Noclip", &mIsEnableNoclip);
        ImGui::SameLine();
        ImGui::Checkbox("Reload Scene at Pos", &mIsReloadPos);
        if (GImGui->NavId == ImGui::GetID("Reload Scene at Pos")) {
            ImGui::SetTooltip("Doesn't save!!! \nWill break normal reload \n(To fix: re enter stage)");
        }
        ImGui::PushItemWidth(200);
        static int txt = set->getSettings()->mMoonRefreshText;
        static int corner = set->getSettings()->mMenuCorner;
        static int key = set->getSettings()->mMenuKey;
        if (ImGui::Combo("Moon Refresh Text", &txt, MoonRefreshTexts, IM_ARRAYSIZE(MoonRefreshTexts)))
            set->getSettings()->mMoonRefreshText = txt;
        if (ImGui::Combo("Menu Corner", &corner, Corners, IM_ARRAYSIZE(Corners)))
            set->getSettings()->mMenuCorner = corner;
        if (ImGui::Combo("Open Menu Key", &key, MenuKeys, IM_ARRAYSIZE(MenuKeys)))
            set->getSettings()->mMenuKey = key;
        ImGui::PopItemWidth();

        ImGui::Unindent();
    }
}
