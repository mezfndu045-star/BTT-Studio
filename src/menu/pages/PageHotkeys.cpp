#include "hk/sail/detail.h"

#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Nerve/Nerve.h"
#include "al/Library/Nerve/NerveKeeper.h"
#include "al/Library/Nerve/NerveStateCtrl.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/Player/PlayerFunction.h"
#include "game/Scene/StageSceneStateStageMap.h"
#include "game/System/GameDataFunction.h"

#include <cxxabi.h>
#include <typeinfo>

#include "helpers/InputHelper.h"
#include "imgui.h"
#include "menu/Menu.h"
#include "src/settings/SettingsMgr.h"

using namespace btt;
using Hotkey = SettingsMgr::SettingsHotkey;

void Menu::drawPageHotkeys() {
    if (ImGui::CollapsingHeader("Hotkeys")) {
        int savePosKey = (u8)set->getSettings()->mSavePosKey;
        int loadPosKey = (u8)set->getSettings()->mLoadPosKey;
        int restartKey = (u8)set->getSettings()->mRestartSceneKey;
        static int killKey = (u8)set->getSettings()->mKillSceneKey;
        static int healKey = (u8)set->getSettings()->mHealMarioKey;
        static int lifeupKey = (u8)set->getSettings()->mLifeUpKey;
        static int prevSceneKey = (u8)set->getSettings()->mPrevSceneKey;
        static int inctpKey = (u8)set->getSettings()->mIncTpIndexKey;
        static int dectpKey = (u8)set->getSettings()->mDecTpIndexKey;
        static int addCoinsKey = (u8)set->getSettings()->mAddCoinsKey;
        static int decCoinsKey = (u8)set->getSettings()->mDecCoinsKey;
        static int incPatternKey = (u8)set->getSettings()->mIncPatternKey;
        static int decPatternKey = (u8)set->getSettings()->mDecPatternKey;
        static int warpCpKey = (u8)set->getSettings()->mWarpLastCpKey;
        static int timerStartKey = (u8)set->getSettings()->mTimerStartKey;
        static int timerEndKey = (u8)set->getSettings()->mTimerEndKey;
        static int timerResetKey = (u8)set->getSettings()->mTimerResetKey;

        ImGui::Indent();
        ImGui::Text("Usage: Hold Hotkey + Press DPad-Up (or custom for Pos/Restart)");
        ImGui::PushItemWidth(200);
        if (ImGui::Combo("Save Position##Key", &savePosKey, TpKeys, IM_ARRAYSIZE(TpKeys)))
            set->getSettings()->mSavePosKey = (SettingsMgr::TpKey)savePosKey;
        if (ImGui::Combo("Load Position##Key", &loadPosKey, TpKeys, IM_ARRAYSIZE(TpKeys)))
            set->getSettings()->mLoadPosKey = (SettingsMgr::TpKey)loadPosKey;
        if (ImGui::Combo("Restart Scene##Key", &restartKey, TpKeys, IM_ARRAYSIZE(TpKeys)))
            set->getSettings()->mRestartSceneKey = (SettingsMgr::TpKey)restartKey;
        if (ImGui::Combo("Heal Mario##Key", &healKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mHealMarioKey = (Hotkey)healKey;

        if (ImGui::Combo("Life Up Heart##Key", &lifeupKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mLifeUpKey = (Hotkey)lifeupKey;
        if (ImGui::Combo("Prev Scene##Key", &prevSceneKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mPrevSceneKey = (Hotkey)prevSceneKey;
        if (ImGui::Combo("Increment Tp Index##Key", &inctpKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mIncTpIndexKey = (Hotkey)inctpKey;
        if (ImGui::Combo("Decrement Tp Index##Key", &dectpKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mDecTpIndexKey = (Hotkey)dectpKey;
        if (ImGui::Combo("Add 1000 Coins##Key", &addCoinsKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mAddCoinsKey = (Hotkey)addCoinsKey;
        if (ImGui::Combo("Remove 1000 Coins##Key", &decCoinsKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mDecCoinsKey = (Hotkey)decCoinsKey;
        if (ImGui::Combo("Next Wiggler Pattern##Key", &incPatternKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mIncPatternKey = (Hotkey)incPatternKey;
        if (ImGui::Combo("Prev Wiggler Pattern##Key", &decPatternKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mDecPatternKey = (Hotkey)decPatternKey;
        if (ImGui::Combo("Warp to last Checkpoint##Key", &warpCpKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mWarpLastCpKey = (Hotkey)warpCpKey;
        if (ImGui::Combo("Start Timer##Key", &timerStartKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mTimerStartKey = (Hotkey)timerStartKey;
        if (ImGui::Combo("End Timer##Key", &timerEndKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mTimerEndKey = (Hotkey)timerEndKey;
        if (ImGui::Combo("Reset Timer##Key", &timerResetKey, Keys, IM_ARRAYSIZE(Keys)))
            set->getSettings()->mTimerResetKey = (Hotkey)timerResetKey;

        ImGui::PopItemWidth();
        ImGui::Unindent();
    }
}

bool Menu::isTpKey(SettingsMgr::TpKey key) {
    if (mIsEnabledMenu || key == SettingsMgr::TpKey::Disabled) return false;

    bool L = InputHelper::isHoldL();
    bool R = InputHelper::isHoldR();
    bool ZL = InputHelper::isHoldZL();
    bool ZR = InputHelper::isHoldZR();

    switch (key) {
    case SettingsMgr::TpKey::DLeft:
        return InputHelper::isPressPadLeft() && !L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::DRight:
        return InputHelper::isPressPadRight() && !L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::DDown:
        return InputHelper::isPressPadDown() && !L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::DUp:
        return InputHelper::isPressPadUp() && !L && !R && !ZL && !ZR;

    case SettingsMgr::TpKey::L_DLeft:
        return InputHelper::isPressPadLeft() && L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::L_DRight:
        return InputHelper::isPressPadRight() && L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::L_DDown:
        return InputHelper::isPressPadDown() && L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::L_DUp:
        return InputHelper::isPressPadUp() && L && !R && !ZL && !ZR;

    case SettingsMgr::TpKey::ZL_DLeft:
        return InputHelper::isPressPadLeft() && ZL && !L && !R && !ZR;
    case SettingsMgr::TpKey::ZL_DRight:
        return InputHelper::isPressPadRight() && ZL && !L && !R && !ZR;
    case SettingsMgr::TpKey::ZL_DDown:
        return InputHelper::isPressPadDown() && ZL && !L && !R && !ZR;
    case SettingsMgr::TpKey::ZL_DUp:
        return InputHelper::isPressPadUp() && ZL && !L && !R && !ZR;

    case SettingsMgr::TpKey::R_DLeft:
        return InputHelper::isPressPadLeft() && R && !L && !ZL && !ZR;
    case SettingsMgr::TpKey::R_DRight:
        return InputHelper::isPressPadRight() && R && !L && !ZL && !ZR;

    case SettingsMgr::TpKey::LStick:
        return InputHelper::isPressStickL() && !L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::RStick:
        return InputHelper::isPressStickR() && !L && !R && !ZL && !ZR;

    case SettingsMgr::TpKey::L_LStick:
        return InputHelper::isPressStickL() && L && !R && !ZL && !ZR;
    case SettingsMgr::TpKey::L_RStick:
        return InputHelper::isPressStickR() && L && !R && !ZL && !ZR;

    case SettingsMgr::TpKey::ZL_ZR:
        return (ZL && InputHelper::isPressZR()) || (ZR && InputHelper::isPressZL());

    default:
        return false;
    }
}

bool Menu::isHotkey(Hotkey key) {
    if (mIsEnabledMenu) return false;
    bool isAllowInput = true;
    bool Up = InputHelper::isPressPadUp();
    bool L = InputHelper::isHoldL();
    bool R = InputHelper::isHoldR();
    bool ZL = InputHelper::isHoldZL();
    bool ZR = InputHelper::isHoldZR();
    switch (key) {
    case Hotkey::None: return Up && !L && !R && !ZL && !ZR && isAllowInput;
    case Hotkey::L: return Up && L && !R && !ZL && !ZR && isAllowInput;
    case Hotkey::R: return Up && R && !L && !ZL && !ZR && isAllowInput;
    case Hotkey::ZL: return Up && ZL && !L && !R && !ZR && isAllowInput;
    case Hotkey::ZR: return Up && ZR && !L && !R && !ZL && isAllowInput;
    case Hotkey::LR: return Up && L && R && !ZL && !ZR && isAllowInput;
    case Hotkey::LZL: return Up && L && ZL && !R && !ZR && isAllowInput;
    case Hotkey::LZR: return Up && L && ZR && !R && !ZL && isAllowInput;
    case Hotkey::RZL: return Up && R && ZL && !L && !ZR && isAllowInput;
    case Hotkey::RZR: return Up && R && ZR && !L && !ZL && isAllowInput;
    default: return false;
    }
}

void Menu::triggerReloadPos() {
    if (!stageScene || !playerHak || !gameSeq) return;

    GameDataFunction::recoveryPlayer(playerHak);
    GameDataFunction::recoveryPlayer(playerHak);

    reloadStagePos = al::getTrans(playerHak);
    reloadStageQuat = al::getQuat(playerHak);
    mReloadOldScene = stageScene;
    mReloadState = 1;
    reloadPosTimer = 0;

    ChangeStageInfo info = ChangeStageInfo(
        gameSeq->mGameDataHolderAccessor, "",
        GameDataFunction::getCurrentStageName(gameSeq->mGameDataHolderAccessor), false,
        -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
    );
    gameSeq->mGameDataHolderAccessor.mData->changeNextStage(&info, 0);
}

void Menu::restartScene() {
    if (!stageScene || !playerHak) return;
    al::NerveKeeper* sceneNerveKeeper = stageScene->getNerveKeeper();
    if (!sceneNerveKeeper) return;

    char* stateName = nullptr;
    char* stateNrvName = nullptr;
    char* stateNrvNameShort = nullptr;
    char* stateNameShort = nullptr;
    bool cmpNrv = false;
    bool cmpState = false;

    if (sceneNerveKeeper->mStateCtrl) {
        al::NerveStateCtrl::State* state = sceneNerveKeeper->mStateCtrl->mCurrentState;

        if (state) {
            const al::Nerve* stateNerve = state->state->getNerveKeeper()->getCurrentNerve();
            int status;
            stateName = abi::__cxa_demangle(typeid(*state->state).name(), nullptr, nullptr, &status);
            stateNameShort = stateName + strlen("StageSceneState");

            if (stateName) {
                stateNrvName =
                    abi::__cxa_demangle(typeid(*stateNerve).name(), nullptr, nullptr, &status);

                if (stateNrvName) {
                    auto prefixLen2 = stateNrvName[0] == '(' ? strlen("(anonymous namespace)::") : 0;
                    stateNrvNameShort = stateNrvName + prefixLen2 + strlen(stateName) + strlen("nrv");

                    cmpNrv = strcmp(stateNrvNameShort, "SkipDemo") == 0 ||
                             strcmp(stateNrvNameShort, "Skip") == 0;
                    cmpState = strcmp(stateNameShort, "PauseMenu") == 0 ||
                               strcmp(stateNameShort, "SnapShot") == 0;
                }
            }
        }
        if (stateNrvName) free(stateNrvName);
        if (stateName) free(stateName);
        if (!(cmpNrv || cmpState || PlayerFunction::isPlayerDeadStatus(playerHak))) {
            GameDataFunction::recoveryPlayer(playerHak);
            GameDataFunction::recoveryPlayer(playerHak);

            if (mIsReloadPos) {
                triggerReloadPos();
            } else {
                stageScene->kill();
            }
        }
    }
}

void Menu::handleHotkeys() {
    if (mIsEnabledMenu) return;

    bool isAllowTP = set->getSettings()->mIsEnableTpHotkeys;

    if (isAllowTP) {
        if (isTpKey(set->getSettings()->mSavePosKey)) {
            saveTeleport(tpStates[tpIndex]);
        } else if (isTpKey(set->getSettings()->mLoadPosKey)) {
            loadTeleport(tpStates[tpIndex]);
        }
    }

    if (isTpKey(set->getSettings()->mRestartSceneKey) ||
        isHotkey(set->getSettings()->mKillSceneKey)) {
        restartScene();
    }
    if (isHotkey(set->getSettings()->mHealMarioKey)) {
        if (playerHak) GameDataFunction::recoveryPlayer(playerHak);
    }
    if (isHotkey(set->getSettings()->mPrevSceneKey)) {
        if (stageScene) GameDataHolderAccessor(stageScene)->returnPrevStage();
    }
    if (isHotkey(set->getSettings()->mIncTpIndexKey)) {
        tpIndex++;
        if (tpIndex >= hk::util::arraySize(tpStates)) tpIndex = 0;
        setPopupText("Tp Index: %d", tpIndex);
    }
    if (isHotkey(set->getSettings()->mDecTpIndexKey)) {
        tpIndex--;
        if (tpIndex < 0) tpIndex = hk::util::arraySize(tpStates) - 1;
        setPopupText("Tp Index: %d", tpIndex);
    }
    if (isHotkey(set->getSettings()->mAddCoinsKey)) {
        if (stageScene) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), 1000);
    }
    if (isHotkey(set->getSettings()->mDecCoinsKey)) {
        if (stageScene) {
            int coinNum = GameDataFunction::getCoinNum(GameDataHolderAccessor(stageScene));
            if (coinNum < 1000)
                GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -(1000 - coinNum));
            else
                GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -1000);
        }
    }
    if (isHotkey(set->getSettings()->mLifeUpKey)) {
        if (playerHak) GameDataFunction::getLifeMaxUpItem(playerHak);
    }
    if (isHotkey(set->getSettings()->mIncPatternKey)) {
        set->getSettings()->mWigglerPattern++;
        if (set->getSettings()->mWigglerPattern >= hk::util::arraySize(WigglerPatterns))
            set->getSettings()->mWigglerPattern = 0;
        setPopupText("Wiggler Pattern: %s", WigglerPatterns[set->getSettings()->mWigglerPattern]);
    }
    if (isHotkey(set->getSettings()->mDecPatternKey)) {
        set->getSettings()->mWigglerPattern--;
        if (set->getSettings()->mWigglerPattern < 0)
            set->getSettings()->mWigglerPattern = hk::util::arraySize(WigglerPatterns) - 1;
        setPopupText("Wiggler Pattern: %s", WigglerPatterns[set->getSettings()->mWigglerPattern]);
    }
    if (isHotkey(set->getSettings()->mWarpLastCpKey)) {
        if (stageScene) {
            if (mLastMapTarget) {
                stageScene->mStateCollectionList->mStateStageMap->mMapThing = mLastMapTarget;
                ptr addr = hk::sail::lookupSymbolFromDb<>("StageSceneNrvWarpToCheckpoint");
                stageScene->getNerveKeeper()->setNerve((al::Nerve*)addr);
            }
        }
    }
    if (isHotkey(set->getSettings()->mTimerStartKey)) {
        if (Timer::sInstance) {
            Timer::sInstance->start();
            setPopupText("Timer started");
        }
    }
    if (isHotkey(set->getSettings()->mTimerEndKey)) {
        if (Timer::sInstance) {
            Timer::sInstance->stop();
            setPopupText("Timer stopped");
        }
    }
    if (isHotkey(set->getSettings()->mTimerResetKey)) {
        if (Timer::sInstance) {
            Timer::sInstance->reset();
            setPopupText("Timer reset");
        }
    }
}
