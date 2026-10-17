#include "settings/SettingsHooks.h"

#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/ro/RoUtil.h"
#include "hk/sail/detail.h"

#include "al/Library/Bgm/BgmLineFunction.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveKeeper.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "helpers/getHelper.h"
#include "al/Project/Scene/SceneInitInfo.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/Nerve/Nerve.h"
#include <typeinfo>
#include "al/Library/Player/PlayerUtil.h"

#include "custom/game/MapObj/DoorAreaChange.h"
#include "custom/game/Scene/StageSceneStateStageMap.h"
#include "game/Item/CoinStackGroup.h"
#include "game/MapObj/ChangeStageInfo.h"
#include <new>
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/System/GameDataUtil.h"
#include "game/System/PlayerHitPointData.h"

#include "helpers/InputHelper.h"
#include "helpers/ProgressSnapshot.h"
#include "menu/DebugCommands.h"
#include "logger.h"
#include "game/Item/ShineInfo.h"
#include "menu/Menu.h"
#include "menu/MoonNames.h"
#include "menu/Timer.h"
#include "settings/DemoHooks.hpp"
#include "src/settings/SettingsMgr.h"

using namespace hk;
using namespace btt;

class ShineInfo;
class ShineChip;
class GrowFlowerPot;
class CheckpointFlag;

// Moons collected look new again (Gray Moon Refresh), and with Moon Refresh a moon collected stays collectable
HkTrampoline GreyShineRefreshHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                       const ShineInfo* shineInfo) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s->mIsEnableGrayMoonRefresh || s->mIsEnableMoonRefresh) ? false : orig(writer, shineInfo);
};

HkTrampoline ShineRefreshHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                   const ShineInfo* shineInfo) -> void {
    // Remembered for Misc > Rename Last Moon
    if (shineInfo) btt::onMoonGot(shineInfo->getStageName(), shineInfo->getObjId());
    ptr addr = sail::lookupSymbolFromDb<>("MoonRefreshText");
    ptr offset = addr - ro::getMainModule()->range().start();
    const char* text = Menu::instance()->getMoonRefreshText();
    ro::getMainModule()->writeRo(offset, text, strlen(text) + 1);
    // The moon is always recorded: the moon counter goes up and a story moon moves the story on, like in the normal
    // game. Moon Refresh keeps it collectable as a new moon (GreyShineRefreshHook); the game itself only counts a moon
    // that is not recorded yet.
    orig(writer, shineInfo);
};

// Open Outfit Doors: the guards of the outfit rooms check the outfit in their talk (event flow nodes CheckCostume,
// CheckCostumePair, CheckCostumeMissMatchPart: case 0 = the right outfit). With the option, case 0 every time.
extern "C" s32 al_getCaseEventNextId(const void* node, s32 caseIndex) asm("_ZN2al18getCaseEventNextIdEPKNS_13EventFlowNodeEi");
static bool isOpenOutfitDoors() {
    auto* s = SettingsMgr::getSettings();
    return s && s->mIsEnableOpenOutfitDoors;
}
HkTrampoline checkCostumeNextHook = [](TrampolineStatic(), const void* node) -> s32 {
    return isOpenOutfitDoors() ? al_getCaseEventNextId(node, 0) : orig(node);
};
HkTrampoline checkCostumePairNextHook = [](TrampolineStatic(), const void* node) -> s32 {
    return isOpenOutfitDoors() ? al_getCaseEventNextId(node, 0) : orig(node);
};
HkTrampoline checkCostumeMissMatchNextHook = [](TrampolineStatic(), const void* node) -> s32 {
    return isOpenOutfitDoors() ? al_getCaseEventNextId(node, 0) : orig(node);
};

// Bosses with a GateKeeperDataHolder (Broodals: Topper, Hariet, Spewart, Rango...): "already dead" is their moon got and
// their defeat saved (rs::isAlreadyDeadGK), then they are not placed again. With Moon Refresh / Gray Moon Refresh the
// boss comes back with its moon; with Cutscene Refresh its battle start cutscene too (rs::isAlreadyShowDemoBossBattleStart).
HkTrampoline gateKeeperDeadHook = [](TrampolineStatic(), const void* holder) -> bool {
    auto* s = SettingsMgr::getSettings();
    if (s && (s->mIsEnableMoonRefresh || s->mIsEnableGrayMoonRefresh)) return false;
    return orig(holder);
};
HkTrampoline gateKeeperStartDemoHook = [](TrampolineStatic(), const void* holder) -> bool {
    auto* s = SettingsMgr::getSettings();
    if (s && s->mIsEnableRefreshNpc) return false;
    return orig(holder);
};
// A Broodal fight's start cutscene can only be skipped once it was seen (StateGateKeeperBattleStartDemo::isFirstDemo
// = the save never showed it). With Moon Refresh / Gray Moon Refresh, Restart Scene puts the save back as it was at
// the start of the scene (cutscene not seen yet), and Cutscene Refresh makes it unseen: it can always be skipped then.
static bool isBroodalStartDemoAlwaysSkippable() {
    auto* s = SettingsMgr::getSettings();
    return s && (s->mIsEnableMoonRefresh || s->mIsEnableGrayMoonRefresh || s->mIsEnableRefreshNpc);
}
HkTrampoline gateKeeperFirstDemoHook = [](TrampolineStatic(), const void* state) -> bool {
    return isBroodalStartDemoAlwaysSkippable() ? false : orig(state);
};
HkTrampoline gateKeeperFirstDemoThunkHook = [](TrampolineStatic(), const void* state) -> bool {
    return isBroodalStartDemoAlwaysSkippable() ? false : orig(state);
};

// Items of type DotMarioCat roll al::isPercentProbability(10.0f) at init to become
// "ドットキャラクター(レア)" (8-bit Luigi popping out of the ground) instead of a coin.
bool luigiRareRoll(f32 percent) {
    return SettingsMgr::instance()->getSettings()->mIsEnableLuigiPixels || al::isPercentProbability(percent);
}

// Skip Broodals: once a Broodal fight's start cutscene ends, finish the boss through its own
// last-hit code so the game plays the defeat cutscene and spawns the Multi Moon as usual.
#if BTT_V130
// 1.3 inlines BombTail::damageAndSetNerve and FireBlower::damageOrDie everywhere: their last hit is redone here
extern "C" void al_killForceBeforeDemo(void* actor) asm("_ZN2al19killForceBeforeDemoEPNS_9LiveActorE");
extern "C" void BombTailBombHolder_kill(void* holder) asm("_ZN18BombTailBombHolder4killEv");
static const al::Nerve* getNerveFromDb(const char* name);

static void BombTail_damageAndSetNerve(al::LiveActor* boss) {
    u8* p = reinterpret_cast<u8*>(boss);
    *reinterpret_cast<s32*>(p + 0x108) = 0;
    s32 partNum = *reinterpret_cast<s32*>(p + 0x168);
    void** parts = *reinterpret_cast<void***>(p + 0x170);
    for (s32 i = 0; i < partNum; i++) al_killForceBeforeDemo(parts[i]);
    BombTailBombHolder_kill(*reinterpret_cast<void**>(p + 0x188));
    al::setNerve(boss, getNerveFromDb("BombTailNrvBattleEndDemo"));
}
static void FireBlower_damageOrDie(al::LiveActor* boss) {
    *reinterpret_cast<s32*>(reinterpret_cast<u8*>(boss) + 0x108) = 0;
    al::setNerve(boss, getNerveFromDb("FireBlowerNrvBattleEndDemo"));
}
#else
extern "C" void BombTail_damageAndSetNerve(al::LiveActor* boss) asm("_ZN8BombTail17damageAndSetNerveEv");
extern "C" void FireBlower_damageOrDie(al::LiveActor* boss) asm("_ZN10FireBlower11damageOrDieEv");
#endif

static const al::Nerve* getNerveFromDb(const char* name) {
    return reinterpret_cast<const al::Nerve*>(hk::sail::lookupSymbolFromDb<>(name));
}

// True on the first frame after `boss` leaves its battle start cutscene nerve
static bool isBroodalBattleStarting(al::LiveActor* boss, const char* startDemoNerve) {
    static const al::LiveActor* sBoss = nullptr;
    static bool sWasInStartDemo = false;

    bool isInStartDemo = al::isNerve(boss, getNerveFromDb(startDemoNerve));
    if (boss != sBoss) {
        sBoss = boss;
        sWasInStartDemo = false;
    }
    bool isStarting = sWasInStartDemo && !isInStartDemo;
    sWasInStartDemo = isInStartDemo;
    return isStarting && SettingsMgr::instance()->getSettings()->mIsEnableSkipBroodals;
}

// Hariet / Spewart: hit points at +0x108, the damage function sets the battle end nerve at 0
HkTrampoline bombTailControlHook = [](TrampolineStatic(), al::LiveActor* boss) -> void {
    orig(boss);
    if (isBroodalBattleStarting(boss, "BombTailNrvBattleStartDemo")) {
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(boss) + 0x108) = 1;
        BombTail_damageAndSetNerve(boss);
    }
};

HkTrampoline fireBlowerControlHook = [](TrampolineStatic(), al::LiveActor* boss) -> void {
    orig(boss);
    if (isBroodalBattleStarting(boss, "FireBlowerNrvBattleStartDemo")) {
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(boss) + 0x108) = 1;
        FireBlower_damageOrDie(boss);
    }
};

// Night Metro Multi Moon: Mechawiggler (Mofumofu, only placed in Night Metro) keeps its Multi Moon at +0x1b8
// (rs::initLinkBossShine in Mofumofu::init). As soon as Mario can move, drop it on him the way the boss does after
// its defeat (Shine::appearCommon, then endBossDemoAndStartFall from 500 above the base position at +0x128);
// getting it plays the regular Multi Moon sequence that leads to Day Metro. A plain popup is not enough for a
// boss Multi Moon: it fell through the ground and the get sequence froze.
// Returns true when the moon cannot appear (already appeared or got)
#if BTT_V130
// 1.3 inlines Shine::appearCommon: same check (true when the moon must not appear again). Shine nerves from
// ShineNrvTable: +0 Wait, +0x30..+0x48 the nerves where the moon is already out (with no link at +0x2e0)
extern "C" bool al_isAlive(const void* actor) asm("_ZN2al7isAliveEPKNS_9LiveActorE");
static bool Shine_appearCommon(void* shine) {
    const u8* nerves = reinterpret_cast<const u8*>(hk::sail::lookupSymbolFromDb<>("ShineNrvTable"));
    al::LiveActor* actor = reinterpret_cast<al::LiveActor*>(shine);
    bool isOut = false;
    for (s32 off = 0x30; off <= 0x48 && !isOut; off += 8)
        isOut = al::isNerve(actor, reinterpret_cast<const al::Nerve*>(nerves + off));
    if (isOut && !*reinterpret_cast<void**>(reinterpret_cast<u8*>(shine) + 0x2e0)) return true;
    return al_isAlive(shine) && al::isNerve(actor, reinterpret_cast<const al::Nerve*>(nerves));
}
#else
extern "C" bool Shine_appearCommon(void* shine) asm("_ZN5Shine12appearCommonEv");
#endif
extern "C" void Shine_endBossDemoAndStartFall(void* shine, f32 height) asm("_ZN5Shine23endBossDemoAndStartFallEf");
constexpr static f32 cBossShineFallHeight = 500.0f;
namespace rs {
al::LiveActor* getPlayerActor(const al::Scene* scene);
bool isActiveDemo(const al::Scene* scene);
}

static void* sNightMetroShine = nullptr;
static bool sIsNightMetroShineAppeared = false;

HkTrampoline mofumofuInitHook = [](TrampolineStatic(), al::LiveActor* mofumofu, const al::ActorInitInfo& info) -> void {
    orig(mofumofu, info);
    void* shine = *reinterpret_cast<void**>(reinterpret_cast<u8*>(mofumofu) + 0x1b8);
    if (shine) sNightMetroShine = shine;
};

// Text Refresh: Cappy's texts (start of the kingdoms...) come back. Each one is said once
// (GameDataFunction::isFlagOnTalkMessageInfo): only the ones said since the scene started stay said, so they come
// back at each reload without being repeated in the same scene. Text Refresh off: they all count as said (never shown,
// even on a new save).
static int sSceneTalkFlags[64];
static int sSceneTalkFlagNum = 0;

HkTrampoline talkMessageFlagHook = [](TrampolineStatic(), const al::IUseSceneObjHolder* holder, int index) -> bool {
    auto* s = SettingsMgr::getSettings();
    if (!s) return orig(holder, index);
    if (!s->mIsEnableRefreshWarpText) return true;
    for (int i = 0; i < sSceneTalkFlagNum; i++)
        if (sSceneTalkFlags[i] == index) return true;
    return false;
};
HkTrampoline talkMessageFlagOnHook = [](TrampolineStatic(), const al::IUseSceneObjHolder* holder, int index) -> void {
    orig(holder, index);
    for (int i = 0; i < sSceneTalkFlagNum; i++)
        if (sSceneTalkFlags[i] == index) return;
    if (sSceneTalkFlagNum < int(sizeof(sSceneTalkFlags) / sizeof(sSceneTalkFlags[0])))
        sSceneTalkFlags[sSceneTalkFlagNum++] = index;
};

// Back in a kingdom's main stage from one of its sub-areas (e.g. Luncheon from Slots): its stage start cutscenes are
// not played again (only on arriving in the kingdom or restarting the stage). Set in stageSceneInitHook.
static bool sIsSubAreaReturn = false;
// Mario died: the next load of the stage (from the last checkpoint) does not play its start cutscenes again either
static bool sIsMissReload = false;
namespace btt {
void onStageMiss() {
    sIsMissReload = true;
}
} // namespace btt
static char sLastStage[64] = {};
static s32 sLastWorld = -1;
// Current world of the save (GameDataFile +0x9f0 / 1.3 +0xa38, as GameDataFunction::getCurrentWorldId reads it)
#if BTT_V130
constexpr int cCurrentWorldIdOffset = 0xa38;
#else
constexpr int cCurrentWorldIdOffset = 0x9f0;
#endif
static s32 currentWorldIdOf(GameDataHolder* holder) {
    u8* file = holder ? *reinterpret_cast<u8**>(reinterpret_cast<u8*>(holder) + 0x20) : nullptr;
    return file ? *reinterpret_cast<s32*>(file + cCurrentWorldIdOffset) : -1;
}

// Scenario of the current visit of a kingdom: the scenario its main stage was loaded with (a scenario forced by a
// warp / travel, or the save's). Going into its sub-areas and back keeps it instead of taking the save's scenario again
// (a save further in the story would show what a boss not beaten yet unlocks), and it moves on with the story when a
// quest of that kingdom is done (GameDataFile::setMainScenarioNo). Another kingdom starts a new visit.
static s32 sVisitWorld = -1;
static s32 sVisitScenario = -1;
static s32 sSceneStartVisitScenario = -1;
namespace btt {
// Restart Scene with Moon Refresh puts the story back as it was when the scene started: the visit's scenario too
void restoreVisitScenario() {
    sVisitScenario = sSceneStartVisitScenario;
}
} // namespace btt
HkTrampoline setMainScenarioNoHook = [](TrampolineStatic(), void* file, s32 scenario) -> void {
    orig(file, scenario);
    if (btt::isDebugTrace()) btt::dbgLog("DBG: setMainScenarioNo %d (visit world %d scenario %d)", scenario, sVisitWorld, sVisitScenario);
    if (file && *reinterpret_cast<s32*>(static_cast<u8*>(file) + cCurrentWorldIdOffset) == sVisitWorld && scenario > 0)
        sVisitScenario = scenario;
};
namespace btt {
int getKingdomIndexOfStage(const char* stageName);  // stage_warp.cpp
} // namespace btt

// Same options: the scenario start camera (fly-over with the text of the story at the start of a kingdom / scenario,
// e.g. Sand Kingdom, Night Metro) is only played once (GameDataHolder::isPlayAlreadyScenarioStartCamera)
HkTrampoline scenarioStartCameraHook = [](TrampolineStatic(), const void* holder, int index) -> bool {
    auto* s = SettingsMgr::getSettings();
    if (!sIsSubAreaReturn && s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshKingdomEnter))
        return false;
    return orig(holder, index);
};

// Cutscene Refresh / Kingdom Enter Refresh: the story cutscene at the arrival in a kingdom (e.g. Night Metro, Sand
// Kingdom) is a StageTalkDemoNpcCap placed in the stage. A stage start one (+0x120 -> config, +8) turns itself off
// (+0x128) when its save object (+0x140) says it was already watched, and then skips its event camera and kills
// itself at the end of init: the save object reads "not watched" during its init (isOnSaveObjInfoHook below).
extern "C" bool rs_isOnSaveObjInfo(const void* info) asm("_ZN2rs15isOnSaveObjInfoEPK11SaveObjInfo");
static bool sIsInStageEventDemoInit = false;
// Set when exePlayStage sees Bowser defeated in the Cloud Kingdom: the next travel / Lost Kingdom arrival is the story
// one (crashed Odyssey, see isCrashHomeHook)
static bool sIsCloudCrashArrival = false;
static bool sIsCrashHomeOverride = false;
// Odyssey travel to the Ruined Kingdom at scenario 1: the dragon attack of the story (see isBossAttackedHomeHook)
static bool sIsBossAttackOverride = false;
HkTrampoline stageTalkDemoInitHook = [](TrampolineStatic(), al::LiveActor* demo, const al::ActorInitInfo& info) -> void {
    auto* settings = SettingsMgr::getSettings();
    sIsInStageEventDemoInit =
        sIsCrashHomeOverride ||
        (!sIsSubAreaReturn && settings && (settings->mIsEnableRefreshNpc || settings->mIsEnableRefreshKingdomEnter));
    orig(demo, info);
    sIsInStageEventDemoInit = false;
    u8* p = reinterpret_cast<u8*>(demo);
    const u8* config = *reinterpret_cast<u8**>(p + 0x120);
    void* saveObj = *reinterpret_cast<void**>(p + 0x140);
    bool isWatched = saveObj && rs_isOnSaveObjInfo(saveObj);
    if (btt::isDebugTrace())
        btt::dbgLog("DBG: StageTalkDemoNpcCap %s cfg %d %d %d saveObj %d watched %d enable %d",
                                demo->getName() ? demo->getName() : "?", config ? config[8] : -1, config ? config[9] : -1,
                                config ? config[10] : -1, saveObj != nullptr, isWatched, p[0x128]);
};
// StageTalkDemoNpcCap::tryStartDemo (called when the scene starts the stage start talk) checks the save object again
HkTrampoline stageTalkTryStartHook = [](TrampolineStatic(), al::LiveActor* demo) -> bool {
    auto* s = SettingsMgr::getSettings();
    sIsInStageEventDemoInit =
        sIsCrashHomeOverride || (!sIsSubAreaReturn && s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshKingdomEnter));
    bool isStarted = orig(demo);
    sIsInStageEventDemoInit = false;
    if (btt::isDebugTrace()) btt::dbgLog("DBG: StageTalkDemoNpcCap::tryStartDemo %d", isStarted);
    return isStarted;
};

// Cutscene Refresh / Kingdom Enter Refresh: a scenario start camera (e.g. the Mechawiggler in Night Metro) plays only
// while its linked quest (QuestInfo: +8 number, +0x18 enabled) is active. Once the kingdom's first quest is done, no
// quest is active anymore in that scenario and the camera never comes back: allow the first quest's one then.
extern "C" int rs_getActiveQuestNum(const void* holder) asm("_ZN2rs17getActiveQuestNumEPKN2al18IUseSceneObjHolderE");
HkTrampoline isPlayScenarioCameraHook = [](TrampolineStatic(), const void* file, const u8* quest) -> bool {
    if (orig(file, quest)) return true;
    auto* s = SettingsMgr::getSettings();
    if (sIsSubAreaReturn || !s || !(s->mIsEnableRefreshNpc || s->mIsEnableRefreshKingdomEnter) || !quest) return false;
    return *reinterpret_cast<const s32*>(quest + 8) == 0 && quest[0x18] && rs_getActiveQuestNum(quest) == 0;
};

// After the Bowser fight of the Cloud Kingdom, HakoniwaSequence::exePlayStage crashes the Odyssey and travels to the
// Lost Kingdom with the world warp, which takes the Lost Kingdom scenario of the save. When the fight is replayed on a
// save past it, that is a later scenario: always land in scenario 1, as in the story (pending ChangeStageInfo:
// GameDataFile +0xa30 / 1.3 +0xa78, scenario at +0x1cc).
// That travel only happens while the story is at "Bowser found" (GameProgressData +0x34 == 3, isFindKoopa): on a save
// past it, StageScene::init does not even keep the Bowser of the stage (+0x390, isDefeatKoopaLv1) and finishing the
// fight again would not go to the Lost Kingdom. In a Cloud stage, answer "found" during StageScene::init and once
// Bowser is defeated, so HakoniwaSequence::exePlayStage crashes the Odyssey and travels as in the story.
static bool sIsInStageSceneInit = false;
// The current StageScene, even once it has ended (helpers::tryGetStageScene needs it alive)
static StageScene* getCurrentStageSceneEvenEnded() {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
    al::Scene* curScene = gameSeq ? gameSeq->mCurrentScene : nullptr;
    return curScene && strcmp(curScene->mName.cstr(), "StageScene") == 0 ? static_cast<StageScene*>(curScene) : nullptr;
}
HkTrampoline isFindKoopaHook = [](TrampolineStatic(), GameDataHolder* holder) -> bool {
    if (orig(holder)) return true;
    const char* current = holder ? GameDataFunction::getCurrentStageName(GameDataHolderAccessor(holder)) : nullptr;
    if (!current || strncmp(current, "Cloud", 5) != 0) return false;
    if (sIsInStageSceneInit) return true;
    // exePlayStage checks it once the scene has ended (not alive anymore)
    StageScene* scene = getCurrentStageSceneEvenEnded();
    bool isDefeat = scene && scene->isDefeatKoopaLv1();
    static int sLast = -1;
    if (btt::isDebugTrace() && sLast != (int)isDefeat + (scene ? 2 : 0)) {
        sLast = (int)isDefeat + (scene ? 2 : 0);
        void* koopa = scene ? *reinterpret_cast<void**>(reinterpret_cast<u8*>(scene) + 0x390) : nullptr;
        btt::dbgLog("DBG: isFindKoopa scene %d koopa %p defeat %d", scene != nullptr, koopa, isDefeat);
    }
    if (isDefeat) sIsCloudCrashArrival = sIsCrashHomeOverride = true;
    return isDefeat;
};
// Scenario of an Odyssey travel (world warp): it goes through a cutscene stage (DemoChangeWorldStage,
// DemoCrashHomeFallStage...), then the change to the kingdom takes the scenario of the save again
// (GameDataFunction::calcNextScenarioNo). A forced scenario for the travel's kingdom is kept until it is loaded:
//  - Stages > Warp to Stage with Kingdom Enter Refresh: the scenario chosen in the menu (btt::setModWorldWarpScenario);
//  - the Cloud Kingdom crash: scenario 1 of the Lost Kingdom (story arrival);
//  - Always Odyssey Power Up / Cutscene Refresh: scenario 1 (arrival cutscene, first boss... e.g. Seaside -> Luncheon
//    again for an IL).
static char sForcedScenarioStage[64] = {};
static s32 sForcedScenario = -1;
static s32 sModWorldWarpScenario = -1;
namespace btt {
void setModWorldWarpScenario(s32 scenario) {
    sModWorldWarpScenario = scenario;
}
} // namespace btt
HkTrampoline cloudToLostWarpHook = [](TrampolineStatic(), GameDataHolder* holder, const char* stageName) -> bool {
    s32 modScenario = sModWorldWarpScenario;
    sModWorldWarpScenario = -1;
    auto* settings = SettingsMgr::getSettings();
    bool isCrash = sIsCloudCrashArrival && stageName && strcmp(stageName, "ClashWorldHomeStage") == 0;
    bool isOptionTravel = modScenario < 0 && !isCrash && stageName && settings &&
                          (settings->mIsEnableAlwaysOdysseyPowerUp || settings->mIsEnableRefreshNpc);
    // Moon Refresh / Gray Moon Refresh: going back to that kingdom's scenario 1 also undoes its story moons (the progress
    // from before the first travel there is put back, before the game prepares the travel)
    if (isOptionTravel && (settings->mIsEnableMoonRefresh || settings->mIsEnableGrayMoonRefresh)) {
        bool isRestored = btt::takeOrRestoreTravelSnapshot(holder, stageName);
        if (btt::isDebugTrace()) btt::dbgLog("DBG: travel snapshot %s restored %d", stageName, isRestored);
    }
    bool r = orig(holder, stageName);
    if (!stageName) return r;
    s32 scenario = -1;
    if (modScenario >= 0) scenario = modScenario > 0 ? modScenario : -1;
    else if (isCrash || isOptionTravel) scenario = 1;
    if (btt::isDebugTrace()) btt::dbgLog("DBG: world warp %s scenario %d", stageName, scenario);
    if (scenario == 1 && strcmp(stageName, "BossRaidWorldHomeStage") == 0) sIsBossAttackOverride = true;
    if (scenario <= 0) return r;
    snprintf(sForcedScenarioStage, sizeof(sForcedScenarioStage), "%s", stageName);
    sForcedScenario = scenario;
    u8* file = *reinterpret_cast<u8**>(reinterpret_cast<u8*>(holder) + 0x20);
#if BTT_V130
    constexpr int cChangeStageInfoOffset = 0xa78;
#else
    constexpr int cChangeStageInfoOffset = 0xa30;
#endif
    u8* next = file ? *reinterpret_cast<u8**>(file + cChangeStageInfoOffset) : nullptr;
    if (next) *reinterpret_cast<s32*>(next + 0x1cc) = scenario;
    return r;
};

// The story of that crash (the Odyssey shot down during the travel, then in the Lost Kingdom the intro, Mario getting
// up and Cappy's talk instead of Mario walking out of the Odyssey) plays while the progress is "home crashed"
// (GameProgressData +0x34 == 4, isCrashHome). GameProgressData::crashHome only sets it from "Bowser found": on a save
// past it, the progress stays later. Answer "crashed" (and not "repaired") from the defeat until the Odyssey is
// repaired (repairHome) or Mario leaves the Lost Kingdom. The save itself keeps its progress.
HkTrampoline isCrashHomeHook = [](TrampolineStatic(), GameDataHolder* holder) -> bool {
    if (sIsCrashHomeOverride) return true;
    return orig(holder);
};
// Forced scenario of the Odyssey travel (cloudToLostWarpHook above). Next stage name: GameDataFile +0x870 / 1.3 +0x8b8.
HkTrampoline calcNextScenarioNoHook = [](TrampolineStatic(), GameDataHolder* holder) -> s32 {
    s32 scenario = orig(holder);
    if (!holder) return scenario;
    u8* file = *reinterpret_cast<u8**>(reinterpret_cast<u8*>(holder) + 0x20);
#if BTT_V130
    constexpr int cNextStageNameOffset = 0x8b8;
#else
    constexpr int cNextStageNameOffset = 0x870;
#endif
    const char* next = file ? *reinterpret_cast<const char**>(file + cNextStageNameOffset) : nullptr;
    if (sForcedScenario > 0 && next && strcmp(next, sForcedScenarioStage) == 0) return sForcedScenario;
    // A stage of the kingdom being visited (main stage or sub-area) taking the save's scenario (no scenario given by
    // the stage change: GameDataFile +0xa28, and +0xa24 unless +0xa0a / 1.3 +0xa70, +0xa6c, +0xa52): the visit's one
#if BTT_V130
    constexpr int cScenarioA = 0xa70, cScenarioB = 0xa6c, cScenarioFlag = 0xa52;
#else
    constexpr int cScenarioA = 0xa28, cScenarioB = 0xa24, cScenarioFlag = 0xa0a;
#endif
    bool isSaveScenario = file && *reinterpret_cast<s32*>(file + cScenarioA) == -1 &&
                          (file[cScenarioFlag] || *reinterpret_cast<s32*>(file + cScenarioB) == -1);
    if (isSaveScenario && scenario > 0 && scenario != 10 && sVisitScenario > 0 && next &&
        btt::getKingdomIndexOfStage(next) == sVisitWorld)
        return sVisitScenario;
    return scenario;
};
// The travel to the Ruined Kingdom with the dragon attack (DemoChangeWorldScene / HakoniwaStateDemoWorldWarp: the
// BossRaidAttack cutscene) and the Odyssey's talk there follow the progress "attacked by the boss" (GameProgressData
// +0x34 == 6, isBossAttackedHome), set by GameDataFile::startStage only from "home repaired". On a save past it, answer
// "attacked" for a travel there at scenario 1, until the Odyssey is repaired (repairHomeByCrashedBoss) or Mario leaves
// the Ruined Kingdom. The save keeps its progress.
HkTrampoline isBossAttackedHomeHook = [](TrampolineStatic(), GameDataHolder* holder) -> bool {
    if (sIsBossAttackOverride) return true;
    return orig(holder);
};
HkTrampoline repairHomeByCrashedBossHook = [](TrampolineStatic(), GameDataHolder* holder) -> void {
    sIsBossAttackOverride = false;
    orig(holder);
};
// The objects placed in a stage follow the scenario given by the stage change (ChangeStageInfo +0x1cc, -1 = none): a
// door or pipe gives none, then the placement takes the save's scenario even when the loading one is the visit's (a save
// further in the story would show the enemies of a later scenario). A stage change to the kingdom being visited without
// a scenario gets the visit's one, as an Odyssey travel does.
// The last stage change asked by the game (a door, a pipe...), not by Restart Scene: Restart Scene replays it when the game
// has no entrance to restart at (back from a room: the door of the room makes Mario walk out only for that return)
alignas(8) static u8 sLastChangeInfo[sizeof(ChangeStageInfo)];
static bool sIsLastChangeInfo = false;
static bool sIsModChangeStage = false;
namespace btt {
const ChangeStageInfo* getLastGameChangeStage() {
    return sIsLastChangeInfo ? reinterpret_cast<const ChangeStageInfo*>(sLastChangeInfo) : nullptr;
}
void setModChangeStage(bool isMod) {
    sIsModChangeStage = isMod;
}
} // namespace btt
HkTrampoline changeNextStageHook = [](TrampolineStatic(), GameDataHolder* holder, const ChangeStageInfo* info, s32 raceType) -> void {
    if (info && !sIsModChangeStage && info->getStageName() && info->getStageName()[0]) {
        if (!sIsLastChangeInfo) {
            new (sLastChangeInfo) ChangeStageInfo(holder, "", "", false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
            sIsLastChangeInfo = true;
        }
        reinterpret_cast<ChangeStageInfo*>(sLastChangeInfo)->copy(*info);
    }
    if (info && sVisitScenario > 0) {
        s32* scenario = reinterpret_cast<s32*>(reinterpret_cast<u8*>(const_cast<ChangeStageInfo*>(info)) + 0x1cc);
        const char* stage = info->getStageName();
        if (*scenario <= 0 && stage && stage[0] && btt::getKingdomIndexOfStage(stage) == sVisitWorld) {
            if (btt::isDebugTrace()) btt::dbgLog("DBG: changeNextStage %s scenario %d -> %d", stage, *scenario, sVisitScenario);
            *scenario = sVisitScenario;
        }
    }
    orig(holder, info, raceType);
};
HkTrampoline isRepairHomeHook = [](TrampolineStatic(), GameDataHolder* holder) -> bool {
    if (sIsCrashHomeOverride) return false;
    return orig(holder);
};
HkTrampoline repairHomeHook = [](TrampolineStatic(), GameDataHolder* holder) -> void {
    sIsCrashHomeOverride = false;
    orig(holder);
};

// Arrival by the Odyssey (Kingdom Enter Refresh world warp): after Mario walks out of the Odyssey, the game starts the
// stage start talk of Cappy only after a scenario start camera (exeDemoScenarioCamera). Without one, the demo ends
// (endDemoAndChangeNerve) and the talk never comes: start it there, the way exeDemoScenarioCamera does.
extern "C" bool CapManHeroDemoUtil_isExistTalkDemoStageStart(const al::Scene* scene)
    asm("_ZN18CapManHeroDemoUtil25isExistTalkDemoStageStartEPKN2al5SceneE");
extern "C" void CapManHeroDemoUtil_startTalkDemoStageStart(const al::Scene* scene)
    asm("_ZN18CapManHeroDemoUtil23startTalkDemoStageStartEPKN2al5SceneE");
HkTrampoline endDemoAndChangeNerveHook = [](TrampolineStatic(), StageScene* scene) -> void {
    auto* s = SettingsMgr::getSettings();
    if (s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshKingdomEnter) &&
        al::isNerve(scene, getNerveFromDb("StageSceneNrvDemoAppearFromHome")) &&
        CapManHeroDemoUtil_isExistTalkDemoStageStart(scene)) {
        CapManHeroDemoUtil_startTalkDemoStageStart(scene);
        al::updateKitListPostOnNerveEnd(scene);
        al::setNerve(scene, getNerveFromDb("StageSceneNrvDemoStageStartCapManHeroTalk"));
        return;
    }
    orig(scene);
};

// The story cutscenes at the start of a stage (e.g. Night Metro arrival) are StageEventDemo objects: once watched
// (their save object, rs::isOnSaveObjInfo), StageEventDemo::init does not even create the cutscene. With Cutscene
// Refresh / Kingdom Enter Refresh, the save object reads "not watched" during their init.
HkTrampoline stageEventDemoInitHook = [](TrampolineStatic(), al::LiveActor* demo, const al::ActorInitInfo& info) -> void {
    auto* s = SettingsMgr::getSettings();
    sIsInStageEventDemoInit = !sIsSubAreaReturn && s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshKingdomEnter);
    orig(demo, info);
    sIsInStageEventDemoInit = false;
    if (btt::isDebugTrace())
        btt::dbgLog("DBG: StageEventDemo %s stageStart %d", demo->getName() ? demo->getName() : "?",
                    reinterpret_cast<u8*>(demo)[0x118]);
};
HkTrampoline isOnSaveObjInfoHook = [](TrampolineStatic(), const void* info) -> bool {
    if (sIsInStageEventDemoInit) return false;
    return orig(info);
};

// Pots with a grow level kept in memory by Seeds Refresh (see seedGetGrowLevelHook), reset on each scene
static int sSeedGrowLevelNum = 0;

namespace btt {
void onTimerSceneInit();  // TimerHooks.cpp
} // namespace btt
// Entrances of the stage (PlayerStartInfo, made for every start point while the stage loads: its start id at +0x18
// (string: the given name, else the ChangeStageId of the placement, else its placement id) and its place at +0xb0; doors
// DoorWarp / DoorWarpStageChange give the place Mario walks out to). Restart Scene uses them when the game has no entrance to restart at (back from a room, or after a door that
// leads to another place of the same stage): the stage restarts at the entrance where Mario appeared, with its walk out
// and its camera.
namespace {
struct StageStart {
    char id[64];
    sead::Vector3f trans;
};
StageStart sStageStarts[96];
int sStageStartNum = 0;
int sStageStartCalls = 0;
} // namespace
HkTrampoline playerStartInfoCtorHook = [](TrampolineStatic(), void* info, const void* placement, void* camera,
                                          const char* id, void* startObj, const sead::Vector3f* trans,
                                          const void* quat) -> void {
    orig(info, placement, camera, id, startObj, trans, quat);
    sStageStartCalls++;
    const char* startId = *reinterpret_cast<const char* const*>(static_cast<u8*>(info) + 0x18);
    if (!startId || !startId[0] || sStageStartNum >= int(sizeof(sStageStarts) / sizeof(sStageStarts[0]))) return;
    StageStart& s = sStageStarts[sStageStartNum++];
    snprintf(s.id, sizeof(s.id), "%s", startId);
    s.trans = *reinterpret_cast<const sead::Vector3f*>(static_cast<u8*>(info) + 0xb0);
};
// Doors that lead to another place of the same stage (DoorWarp) of the stage, to send Mario through one again after a
// Restart Scene
al::LiveActor* sStageDoors[64];
int sStageDoorNum = 0;
HkTrampoline doorWarpInitHook = [](TrampolineStatic(), al::LiveActor* door, const al::ActorInitInfo& info) -> void {
    orig(door, info);
    if (sStageDoorNum < int(sizeof(sStageDoors) / sizeof(sStageDoors[0]))) sStageDoors[sStageDoorNum++] = door;
};
extern "C" void* al_getHitSensor(const al::LiveActor* actor, const char* name) asm("_ZN2al12getHitSensorEPKNS_9LiveActorEPKc");
extern "C" void rs_requestBindPlayer(const al::LiveActor* actor, void* sensor)
    asm("_ZN2rs17requestBindPlayerEPKN2al9LiveActorEPNS0_9HitSensorE");
namespace btt {
// Mario goes through the door of the stage nearest to pos (within maxDist), like walking into it; false: none
bool enterStageDoorNear(const sead::Vector3f& pos, float maxDist) {
    al::LiveActor* best = nullptr;
    float bestD = maxDist * maxDist;
    for (int i = 0; i < sStageDoorNum; i++) {
        sead::Vector3f d = al::getTrans(sStageDoors[i]) - pos;
        float dd = d.x * d.x + d.y * d.y + d.z * d.z;
        if (dd < bestD) {
            bestD = dd;
            best = sStageDoors[i];
        }
    }
    if (isDebugTrace()) {
        dbgLog("DBG: %d doors near %.0f %.0f %.0f", sStageDoorNum, pos.x, pos.y, pos.z);
        for (int i = 0; i < sStageDoorNum && i < 20; i++) {
            const sead::Vector3f& t = al::getTrans(sStageDoors[i]);
            dbgLog("DBG:   door %.0f %.0f %.0f", t.x, t.y, t.z);
        }
    }
    void* sensor = best ? al_getHitSensor(best, "Bind") : nullptr;
    if (!sensor) return false;
    rs_requestBindPlayer(best, sensor);
    return true;
}
// Test traces: the entrances noted for this stage
void traceStageStarts() {
    dbgLog("DBG: %d entrances (%d calls)", sStageStartNum, sStageStartCalls);
    for (int i = 0; i < sStageStartNum && i < 40; i++)
        dbgLog("DBG:   %s %.0f %.0f %.0f", sStageStarts[i].id, sStageStarts[i].trans.x, sStageStarts[i].trans.y, sStageStarts[i].trans.z);
}
// The entrance of the stage nearest to pos (within maxDist), or null
const char* findStageStartNear(const sead::Vector3f& pos, float maxDist) {
    const char* best = nullptr;
    float bestD = maxDist * maxDist;
    for (int i = 0; i < sStageStartNum; i++) {
        sead::Vector3f d = sStageStarts[i].trans - pos;
        float dd = d.x * d.x + d.y * d.y + d.z * d.z;
        if (dd < bestD) {
            bestD = dd;
            best = sStageStarts[i].id;
        }
    }
    return best;
}
} // namespace btt

HkTrampoline stageSceneInitHook = [](TrampolineStatic(), StageScene* stageScene, const al::SceneInitInfo& info) -> void {
    // the entrances of the stage are registered again while it loads
    sStageStartNum = 0;
    sStageStartCalls = 0;
    sStageDoorNum = 0;
    btt::onTimerSceneInit();
    sNightMetroShine = nullptr;
    sIsNightMetroShineAppeared = false;
    sSeedGrowLevelNum = 0;
    sSceneTalkFlagNum = 0;
    sIsInStageSceneInit = true;
    {
        // Main stage (…WorldHomeStage) loaded right after a sub-area or zone of the same kingdom (e.g. LavaBonus1Zone):
        // back from a sub-area. The kingdom of the main stage (mainNames index = world id) is compared with the world
        // the previous stage was loaded in (sLastWorld, read once it was initialized). Cutscene stages (Demo*) and
        // other main stages are not sub-areas.
        // The stage being loaded (the GameDataHolder still has the previous one at this point)
        const char* stage = info.initStageName;
        int kingdom = btt::getKingdomIndexOfStage(stage);
        sIsSubAreaReturn = stage && strstr(stage, "WorldHomeStage") && sLastStage[0] && strcmp(stage, sLastStage) != 0 &&
                           !strstr(sLastStage, "WorldHomeStage") && strncmp(sLastStage, "Demo", 4) != 0 &&
                           kingdom >= 0 && kingdom == sLastWorld;
        if (sIsMissReload) sIsSubAreaReturn = true;
        sIsMissReload = false;
        if (btt::isDebugTrace())
            btt::dbgLog("DBG: sub-area return %d (stage %s kingdom %d, last %s world %d)", sIsSubAreaReturn,
                        stage ? stage : "?", kingdom, sLastStage, sLastWorld);
    }
    if (sIsBossAttackOverride) {
        const char* stage = info.initStageName;
        if (!stage || !(strncmp(stage, "Demo", 4) == 0 || strncmp(stage, "BossRaid", 8) == 0))
            sIsBossAttackOverride = false;
    }
    if (sIsCrashHomeOverride) {
        // The scene is not initialized yet: the stage being loaded comes from SceneInitInfo. The crash
        // story lasts through the cutscene stages (Demo*: DemoCrashHomeFallStage) and the Lost Kingdom (Clash*).
        const char* stage = info.initStageName;
        bool isCrashStory = stage && (strncmp(stage, "Demo", 4) == 0 || strncmp(stage, "Clash", 5) == 0 ||
                                      (sIsCloudCrashArrival && strncmp(stage, "Cloud", 5) == 0));
        if (btt::isDebugTrace()) btt::dbgLog("DBG: crash story stage %s keep %d", stage ? stage : "?", isCrashStory);
        if (!isCrashStory) sIsCrashHomeOverride = false;
        if (stage && strncmp(stage, "Clash", 5) == 0) sIsCloudCrashArrival = false;
    }
    orig(stageScene, info);
    sIsInStageSceneInit = false;
    {
        GameDataHolderAccessor accessor(stageScene);
        const char* loaded = GameDataFunction::getCurrentStageName(accessor);
        snprintf(sLastStage, sizeof(sLastStage), "%s", loaded ? loaded : "");
        sLastWorld = currentWorldIdOf(accessor.mData);
        // A main stage sets the scenario of the visit; a stage of another kingdom ends the visit
        if (loaded && strstr(loaded, "WorldHomeStage") && info.scenarioNo > 0) {
            sVisitWorld = sLastWorld;
            sVisitScenario = info.scenarioNo;
        } else if (sLastWorld != sVisitWorld) {
            sVisitWorld = -1;
            sVisitScenario = -1;
        }
        sSceneStartVisitScenario = sVisitScenario;
        if (btt::isDebugTrace())
            btt::dbgLog("DBG: loaded %s scenario %d world %d -> visit world %d scenario %d", loaded ? loaded : "?",
                        info.scenarioNo, sLastWorld, sVisitWorld, sVisitScenario);
    }
    if (sForcedScenario > 0) {
        const char* loaded = GameDataFunction::getCurrentStageName(GameDataHolderAccessor(stageScene));
        if (loaded && strcmp(loaded, sForcedScenarioStage) == 0) sForcedScenario = -1;
    }
    if (btt::isDebugTrace()) {
        GameDataHolderAccessor accessor(stageScene);
#if !BTT_V130
        btt::dbgLog("DBG: init worldWarp %d nerve intro %d appear %d talk %d",
                    GameDataFunction::isPlayDemoWorldWarp(accessor),
                    al::isNerve(stageScene, getNerveFromDb("StageSceneNrvDemoWorldIntroCamera")),
                    al::isNerve(stageScene, getNerveFromDb("StageSceneNrvDemoAppearFromHome")),
                    al::isNerve(stageScene, getNerveFromDb("StageSceneNrvDemoStageStartCapManHeroTalk")));
#endif
        s32 worldId = currentWorldIdOf(accessor.mData);
        btt::dbgLog("DBG: StageScene::init %s world %d unlocked %d scenario %d crash %d",
                    GameDataFunction::getCurrentStageName(accessor), worldId,
                    GameDataFunction::isUnlockedWorld(accessor, worldId < 0 ? 0 : worldId),
                    worldId < 0 ? -1 : GameDataFunction::getWorldScenarioNo(accessor, worldId), sIsCrashHomeOverride);
    }
        // Moon Refresh: the progress at the start of the scene, put back by Restart Scene
    btt::takeProgressSnapshot(GameDataHolderAccessor(stageScene).mData);
};

static void tryAppearNightMetroShine(StageScene* stageScene) {
    if (!sNightMetroShine || sIsNightMetroShineAppeared) return;
    if (!SettingsMgr::instance()->getSettings()->mIsEnableNightMetroMultiMoon) return;
    if (!al::isNerve(stageScene, getNerveFromDb("StageSceneNrvPlay")) || rs::isActiveDemo(stageScene)) return;
    al::LiveActor* player = rs::getPlayerActor(stageScene);
    if (!player) return;
    sIsNightMetroShineAppeared = true;
    if (Shine_appearCommon(sNightMetroShine)) return;
    // Same steps as Shine::appearAndJoinBossDemo, without joining the boss defeat demo
    al::LiveActor* shine = reinterpret_cast<al::LiveActor*>(sNightMetroShine);
    shine->appear();
    al::invalidateClipping(shine);
    *reinterpret_cast<sead::Vector3f*>(reinterpret_cast<u8*>(sNightMetroShine) + 0x128) = al::getTrans(player);
    Shine_endBossDemoAndStartFall(sNightMetroShine, cBossShineFallHeight);
}

// Rango: hit points at +0x158, the last stomp goes straight to the battle end nerve
HkTrampoline capThrowerControlHook = [](TrampolineStatic(), al::LiveActor* boss) -> void {
    orig(boss);
    if (isBroodalBattleStarting(boss, "CapThrowerNrvBattleStartDemo")) {
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(boss) + 0x158) = 0;
        al::setNerve(boss, getNerveFromDb("CapThrowerNrvBattleEndDemo"));
    }
};

// Madame Broode: DamageLast ends the boss battle and leads to the battle end cutscene
HkTrampoline breedaControlHook = [](TrampolineStatic(), al::LiveActor* boss) -> void {
    orig(boss);
    if (isBroodalBattleStarting(boss, "BreedaNrvDemoBattleStart"))
        al::setNerve(boss, getNerveFromDb("BreedaNrvDamageLast"));
};

// Always Odyssey Power Up: ShineTowerRocket asks "do the moons being paid reach the next unlock?" at several
// points of the pay sequence. Answer yes for any payment so the power up cutscene plays, but only until it has
// started (tryLevelUp succeeded): the game asks again afterwards to chain levels, which would loop forever.
// Without any moon to pay, the Odyssey is told it has 1 so the pay sequence starts; the count is set back to 0
// right before tryLevelUp pays it, so no moon is added to the save.
static bool sIsOdysseyPowerUpDone = false;
static bool sIsOdysseyFakePayment = false;
// Mario spawns next to the Odyssey when arriving in a kingdom: a payment without moons there would start the pay
// sequence on top of the arrival camera, so it waits until Mario has walked away from the Odyssey once
static bool sIsOdysseyPlayerAwayOnce = false;
constexpr static f32 cOdysseyAwayDistance = 1500.0f;
// Set when a power up happens with the option on: the world map then heads to the kingdom after the current one
// instead of the furthest kingdom of the save (on an advanced save the game's own check already says yes)
static bool sIsOdysseyPowerUpForced = false;
// Kingdom the power up happened in: by the time decideWorldMap runs, the current world already is the map selection
static s32 sOdysseyPowerUpWorldId = -1;
// Kingdom the Odyssey was sent to, also forced into the world map selection when the map closes
static s32 sOdysseyRedirectWorldId = -1;
// World map opened by a forced power up: its newly unlocked kingdom reveal shows the next story kingdom
static bool sIsOdysseyWorldMapForced = false;
// The destination of a forced trip is not unlocked in the save yet: let the world map unlock it like a regular
// trip to a new kingdom, otherwise its story quests (and the scenario camera) stay inactive
static bool sIsOdysseyRedirectNeedsUnlock = false;

s32 odysseyGetCurrentShineNum(GameDataHolderAccessor accessor) {
    s32 shineNum = GameDataFunction::getCurrentShineNum(accessor);
    sIsOdysseyFakePayment =
        SettingsMgr::instance()->getSettings()->mIsEnableAlwaysOdysseyPowerUp && shineNum < 1 && sIsOdysseyPlayerAwayOnce;
    return sIsOdysseyFakePayment ? 1 : shineNum;
}

HkTrampoline odysseyIsLevelUpHook = [](TrampolineStatic(), al::LiveActor* rocket, s32 worldId, s32 payShineNum) -> bool {
    bool isLevelUp = orig(rocket, worldId, payShineNum);
    if (!SettingsMgr::instance()->getSettings()->mIsEnableAlwaysOdysseyPowerUp) return isLevelUp;
    if ((payShineNum > 0 || sIsOdysseyFakePayment) && !sIsOdysseyPowerUpDone) isLevelUp = true;
    if (isLevelUp) {
        sIsOdysseyPowerUpForced = true;
        sOdysseyPowerUpWorldId = worldId;
    }
    return isLevelUp;
};

// Story order by world index (Hat 0, Waterfall 1, Sand 2, Forest 3, Lake 4, Cloud 5, Clash 6, City 7, Sea 8,
// Snow 9, Lava 10, Boss 11, Sky 12, Moon 13): Cap > Cascade > Sand > Lake > Wooded > Cloud > Lost > Metro >
// Snow > Seaside > Luncheon > Ruined > Bowser > Moon
static s32 getStoryNextWorldId(s32 worldId) {
    constexpr static s32 sNextWorldIds[] = { 1, 2, 4, 5, 3, 6, 7, 9, 10, 8, 11, 12, 13 };
    if (worldId < 0 || worldId >= (s32)(sizeof(sNextWorldIds) / sizeof(sNextWorldIds[0]))) return -1;
    return sNextWorldIds[worldId];
}

static s32 getOdysseyForcedNextWorldId() {
    return sIsOdysseyPowerUpForced ? getStoryNextWorldId(sOdysseyPowerUpWorldId) : -1;
}

// After a power up the map reveals the next locked kingdoms: make it the next story kingdom
HkTrampoline nextLockedWorldNumSceneHook = [](TrampolineStatic(), const void* scene) -> s32 {
    return getOdysseyForcedNextWorldId() >= 0 ? 1 : orig(scene);
};
HkTrampoline nextLockedWorldNumLayoutHook = [](TrampolineStatic(), const void* layout) -> s32 {
    return getOdysseyForcedNextWorldId() >= 0 ? 1 : orig(layout);
};
HkTrampoline nextLockedWorldIdSceneHook = [](TrampolineStatic(), const void* scene, s32 index) -> s32 {
    s32 nextWorldId = getOdysseyForcedNextWorldId();
    return nextWorldId >= 0 ? nextWorldId : orig(scene, index);
};
HkTrampoline nextLockedWorldIdLayoutHook = [](TrampolineStatic(), const void* layout, s32 index) -> s32 {
    s32 nextWorldId = getOdysseyForcedNextWorldId();
    return nextWorldId >= 0 ? nextWorldId : orig(layout, index);
};
// With all 17 kingdoms unlocked (100% save) StageSceneStateWorldMap::appear skips the reveal entirely
HkTrampoline unlockWorldNumForWorldMapHook = [](TrampolineStatic(), const void* scene) -> s32 {
    s32 num = orig(scene);
    return getOdysseyForcedNextWorldId() >= 0 && num >= 17 ? 16 : num;
};

// StageSceneStateWorldMap: selected kingdom at +0xfc, unlocked kingdom count at +0x104, cursor index at +0x108,
// kingdom ids at +0x110, revealed kingdom count at +0x118 (1.3: every field 8 bytes lower)
#if BTT_V130
constexpr static s32 cWorldMapOffsetShift = -8;
#else
constexpr static s32 cWorldMapOffsetShift = 0;
#endif
constexpr static s32 cWorldMapSelectedWorldOffset = 0xfc + cWorldMapOffsetShift;
constexpr static s32 cWorldMapUnlockNumOffset = 0x104 + cWorldMapOffsetShift;
constexpr static s32 cWorldMapCursorOffset = 0x108 + cWorldMapOffsetShift;
constexpr static s32 cWorldMapWorldIdsOffset = 0x110 + cWorldMapOffsetShift;
constexpr static s32 cWorldMapRevealNumOffset = 0x118 + cWorldMapOffsetShift;
HkTrampoline worldMapAppearHook = [](TrampolineStatic(), void* worldMapState) -> void {
    s32 nextWorldId = getOdysseyForcedNextWorldId();
    if (nextWorldId >= 0) sIsOdysseyWorldMapForced = true;
    orig(worldMapState);
    if (nextWorldId < 0) return;
    u8* state = reinterpret_cast<u8*>(worldMapState);
    s32 count = *reinterpret_cast<s32*>(state + cWorldMapUnlockNumOffset);
    s32* worldIds = *reinterpret_cast<s32**>(state + cWorldMapWorldIdsOffset);
    for (s32 i = 0; worldIds && i < count; i++)
        if (worldIds[i] == nextWorldId) *reinterpret_cast<s32*>(state + cWorldMapCursorOffset) = i;
};

// The story quests of a kingdom (and so its scenario start camera) follow the kingdom main scenario stored in
// the save: GameDataHolder +0x20 -> GameDataFile, +0x20 -> s32 main scenario per world index. A kingdom the save
// has not reached yet has none (-1), so give the destination of a forced trip its first scenario (0, quests are 0-based)
static void tryStartOdysseyDestinationScenario(al::LiveActor* rocket, s32 worldId) {
    GameDataHolderAccessor accessor(rocket);
    u8* gameDataFile = *reinterpret_cast<u8**>(reinterpret_cast<u8*>(accessor.mData) + 0x20);
    if (!gameDataFile) return;
    s32* mainScenarioNos = *reinterpret_cast<s32**>(gameDataFile + 0x20);
    if (!mainScenarioNos) return;
    if (mainScenarioNos[worldId] < 0) mainScenarioNos[worldId] = 0;
}

HkTrampoline odysseyDecideWorldMapHook = [](TrampolineStatic(), al::LiveActor* rocket, s32 worldId) -> void {
    if (sIsOdysseyPowerUpForced) {
        sIsOdysseyPowerUpForced = false;
        s32 nextWorldId = getStoryNextWorldId(sOdysseyPowerUpWorldId);
        if (nextWorldId >= 0) {
            worldId = nextWorldId;
            sOdysseyRedirectWorldId = nextWorldId;
            sIsOdysseyRedirectNeedsUnlock = !GameDataFunction::isUnlockedWorld(GameDataHolderAccessor(rocket), nextWorldId);
            tryStartOdysseyDestinationScenario(rocket, nextWorldId);
        }
    }
    orig(rocket, worldId);
};

HkTrampoline odysseyCancelWorldMapHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    sIsOdysseyPowerUpForced = false;
    orig(rocket);
};

// StageSceneStateWorldMap::kill applies the story rules (Odyssey attacked on the way to Bowser's, kingdoms
// unlocked by the power up) to its own selection (+0xfc). Give it the redirected kingdom and no newly unlocked
// kingdom (+0x118) so a forced power up never changes the save's progress.
// While a forced trip leaves the world map, ignore the story events tied to the destination: chasing Bowser's
// ship to the Cloud Kingdom (findKoopa) and the Odyssey attacked on the way to Bowser's Kingdom (bossAttackHome).
// Their scripted arrival never starts after a forced trip, which left the Odyssey waiting and the camera locked.
static bool sIsOdysseyIgnoreStoryTrip = false;

HkTrampoline isFindKoopaNextHook = [](TrampolineStatic(), const void* holder, s32 worldId) -> bool {
    bool isNext = orig(holder, worldId);
    return sIsOdysseyIgnoreStoryTrip ? false : isNext;
};
HkTrampoline isBossAttackedHomeNextHook = [](TrampolineStatic(), const void* holder, s32 worldId) -> bool {
    bool isNext = orig(holder, worldId);
    return sIsOdysseyIgnoreStoryTrip ? false : isNext;
};

// At the story scenario where Bowser's ship leaves the Sand Kingdom, the power up message starts the
// "ship flies away" sequence (KoopaShip::startDemoHomeFlyAway). After a forced power up the arrival in the next
// kingdom never starts and the camera stays locked: keep the regular fade to the world map instead.
HkTrampoline koopaShipFlyAwayHook = [](TrampolineStatic(), al::LiveActor* rocket) -> bool {
    if (sIsOdysseyPowerUpForced) return false;
    return orig(rocket);
};

HkTrampoline worldMapKillHook = [](TrampolineStatic(), void* worldMapState) -> void {
    bool isForcedTrip = sIsOdysseyWorldMapForced;
    if (sIsOdysseyWorldMapForced) {
        sIsOdysseyWorldMapForced = false;
        // The revealed kingdom list (+0x118 count) only holds the destination: unlock nothing else
        if (!sIsOdysseyRedirectNeedsUnlock || sOdysseyRedirectWorldId < 0)
            *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + cWorldMapRevealNumOffset) = 0;
        sIsOdysseyRedirectNeedsUnlock = false;
    }
    if (sOdysseyRedirectWorldId >= 0) {
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + cWorldMapSelectedWorldOffset) =
            sOdysseyRedirectWorldId;
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + cWorldMapRevealNumOffset) = 0;
        sOdysseyRedirectWorldId = -1;
    }
    sIsOdysseyIgnoreStoryTrip = isForcedTrip;
    orig(worldMapState);
    sIsOdysseyIgnoreStoryTrip = false;
};

HkTrampoline odysseyInitHook = [](TrampolineStatic(), al::LiveActor* rocket, const al::ActorInitInfo& info) -> void {
    sIsOdysseyPowerUpForced = false;
    sIsOdysseyPowerUpDone = false;
    sOdysseyRedirectWorldId = -1;
    sIsOdysseyWorldMapForced = false;
    sIsOdysseyFakePayment = false;
    sIsOdysseyPlayerAwayOnce = false;
    orig(rocket, info);
};

// ShineTowerRocket::mPayShineNum, read by tryLevelUp for addPayShine
static s32& getOdysseyPayShineNum(al::LiveActor* rocket) {
    return *reinterpret_cast<s32*>(reinterpret_cast<u8*>(rocket) + 0x12c);
}

HkTrampoline odysseyTryLevelUpHook = [](TrampolineStatic(), al::LiveActor* rocket) -> bool {
    if (sIsOdysseyFakePayment) getOdysseyPayShineNum(rocket) = 0;
    bool isLevelUp = orig(rocket);
    sIsOdysseyFakePayment = false;
    if (isLevelUp) {
        sIsOdysseyPowerUpDone = true;
        if (SettingsMgr::instance()->getSettings()->mIsEnableAlwaysOdysseyPowerUp) sIsOdysseyPowerUpForced = true;
    }
    return isLevelUp;
};

// Odyssey In All Kingdoms: ShineTowerRocket::init removes the Odyssey when the current kingdom is not unlocked
// in the save (for example after teleporting there from the Stages menu)
bool odysseyInitIsUnlockedWorld(GameDataHolderAccessor accessor, s32 worldId) {
    return SettingsMgr::instance()->getSettings()->mIsEnableOdysseyInAllKingdoms ||
           GameDataFunction::isUnlockedWorld(accessor, worldId);
}

// Safety net: if the scene never starts the Odyssey landing, the Odyssey stays hidden in its "wait for the
// landing demo" nerve and the entrance camera stays locked. Unlock the camera and put the Odyssey back in its
// regular wait once no demo has been playing for a while.
namespace rs {
bool isActiveDemo(const al::LiveActor* actor);
}

constexpr static s32 cOdysseyLandingTimeoutFrames = 180;
static s32 sOdysseyLandingWaitFrames = 0;

HkTrampoline odysseyControlHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    orig(rocket);
    if (!SettingsMgr::instance()->getSettings()->mIsEnableAlwaysOdysseyPowerUp ||
        !al::isNerve(rocket, getNerveFromDb("OdysseyNrvWaitLanding"))) {
        sOdysseyLandingWaitFrames = 0;
        return;
    }
    // The kingdom intro cutscene plays while the Odyssey waits for its landing: only count frames without a demo
    if (rs::isActiveDemo(rocket)) {
        sOdysseyLandingWaitFrames = 0;
        return;
    }
    if (++sOdysseyLandingWaitFrames < cOdysseyLandingTimeoutFrames) return;
    sOdysseyLandingWaitFrames = 0;
    al::validateEndEntranceCamera(rocket);
    al::showModelIfHide(rocket);
    al::setNerve(rocket, getNerveFromDb("OdysseyNrvWait"));
};

// StageScene::init only plays the kingdom intro (then the Odyssey landing) in unlocked kingdoms: a forced trip
// to a kingdom the save has not reached yet would start the stage without them
bool stageSceneInitIsUnlockedWorld(GameDataHolderAccessor accessor, s32 worldId) {
    return SettingsMgr::instance()->getSettings()->mIsEnableAlwaysOdysseyPowerUp ||
           GameDataFunction::isUnlockedWorld(accessor, worldId);
}

// The map only offers the warp to the Odyssey when GameDataFunction::isExistHome, which also needs the current
// kingdom to be unlocked in the save: with Odyssey In All Kingdoms, an activated Odyssey is enough
HkTrampoline isExistHomeHook = [](TrampolineStatic(), GameDataHolderAccessor accessor) -> bool {
    if (orig(accessor)) return true;
    return SettingsMgr::instance()->getSettings()->mIsEnableOdysseyInAllKingdoms &&
           GameDataFunction::isActivateHome(accessor);
};

HkTrampoline odysseyWaitHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    sIsOdysseyPowerUpDone = false;
    if (!al::isNearPlayer(rocket, cOdysseyAwayDistance)) sIsOdysseyPlayerAwayOnce = true;
    orig(rocket);
};

HkTrampoline stageSceneControlHook = [](TrampolineStatic(), StageScene* stageScene) -> void {
    if (SettingsMgr::instance()->getSettings()->mIsEnableDisableMusic) {
        if (al::isPlayingBgm(stageScene)) al::stopAllBgm(stageScene, 0);
    }

    orig(stageScene);
    tryAppearNightMetroShine(stageScene);
};

HkTrampoline saveHook = [](TrampolineStatic(), StageScene* scene) -> bool {
    return SettingsMgr::instance()->getSettings()->mIsEnableDisableAutoSave ? false : orig(scene);
};

HkTrampoline checkpointWarpHook = [](TrampolineStatic(), void* thisPtr) -> bool {
    return SettingsMgr::instance()->getSettings()->mIsEnableAlwaysCheckpoints ? true : orig(thisPtr);
};

HkTrampoline disableMoonLockHook = [](TrampolineStatic(), GameDataHolder* thisPtr, bool* isCrashList,
                                      int worldID) -> int {
    int lockSize = orig(thisPtr, isCrashList, worldID);

    return SettingsMgr::instance()->getSettings()->mIsEnableDisableMoonLock ? 0 : lockSize;
};

HkTrampoline NoDamageHook = [](TrampolineStatic(), PlayerHitPointData* hitPointData) -> void {
    // Timer "Damage" type: Mario got hit, even when No Damage keeps his health
    if (Timer::sInstance) Timer::sInstance->event(TimerHookType::Damage);
    if (!SettingsMgr::instance()->getSettings()->mIsEnableNoDamage) orig(hitPointData);
};

// Text Refresh: Cappy explains the checkpoint flags (warp) at each flag; off: never
HkTrampoline warpTextHook = [](TrampolineStatic(), GameDataHolderAccessor accessor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return s ? !s->mIsEnableRefreshWarpText : orig(accessor);
};

HkTrampoline isEnableShowExplainHook = [](TrampolineStatic(), GameDataHolderAccessor accessor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return s ? s->mIsEnableRefreshWarpText : orig(accessor);
};

HkTrampoline gameDataFileAlreadyShowExplainHook = [](TrampolineStatic(), void* file) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return s ? !s->mIsEnableRefreshWarpText : orig(file);
};

HkTrampoline refreshPurpsHook = [](TrampolineStatic(), GameDataHolderAccessor accessor,
                                   al::ActorInitInfo* actorInitInfo) -> bool {
    return SettingsMgr::instance()->getSettings()->mIsEnableRefreshPurps ? false :
                                                                           orig(accessor, actorInitInfo);
};

HkTrampoline doorRefreshHook = [](TrampolineStatic(), DoorAreaChange* doorAreaChange,
                                  al::ActorInitInfo info) -> void {
    orig(doorAreaChange, info);

    if (SettingsMgr::instance()->getSettings()->mIsEnableDoorRefresh) doorAreaChange->switchCloseAgain();
};

HkTrampoline checkpointFlagHook = [](TrampolineStatic(), CheckpointFlag* checkpointFlag) -> void {
    auto* s = SettingsMgr::instance()->getSettings();
    if (!s || !s->mIsEnableNoCheckpointTouch) orig(checkpointFlag);
};

// Skip Cloud: in a Cloud stage, Bowser (KoopaLv1) is beaten as soon as his battle start cutscene (DemoStart, his first
// nerve) ends: no hit left (KoopaLv1 +0x120 -> KoopaNumberCounter, +4 hits left, 3 at the start) and the nerve the
// last punch leads to (DeadAndDemoEnd: defeat, then the battle end cutscene).
HkTrampoline koopaLv1MovementHook = [](TrampolineStatic(), al::LiveActor* koopa) -> void {
    orig(koopa);
    u8* counter = *reinterpret_cast<u8**>(reinterpret_cast<u8*>(koopa) + 0x120);
    // Test channel "cloudwin": defeat Bowser now
    if (btt::takeDbgCloudWin() && counter) {
        btt::dbgLog("DBG: cloudwin hits %d koopa %p", *reinterpret_cast<s32*>(counter + 4), koopa);
        *reinterpret_cast<s32*>(counter + 4) = 0;
        al::setNerve(koopa, getNerveFromDb("KoopaLv1NrvDeadAndDemoEnd"));
        return;
    }
    if (!SettingsMgr::instance()->getSettings()->mIsEnableSkipCloud) return;
    if (!counter || *reinterpret_cast<s32*>(counter + 4) != 3) return;
    if (al::isNerve(koopa, getNerveFromDb("KoopaLv1NrvDemoStart"))) return;
    const char* stageName = GameDataFunction::getCurrentStageName(GameDataHolderAccessor(koopa));
    if (!stageName || strncmp(stageName, "Cloud", 5) != 0) return;
    *reinterpret_cast<s32*>(counter + 4) = 0;
    al::setNerve(koopa, getNerveFromDb("KoopaLv1NrvDeadAndDemoEnd"));
};

struct MofumofuPatternEntry {
    const char* typeStr;
    int target;
    bool reverse;
};

constexpr static const MofumofuPatternEntry mPatternEntries[22] = {
    { "Ghost", 0, false },   { "Nose", 0, true },     { "C", 1, false },        { "W", 1, true },
    { "J", 2, false },       { "Medal", 2, true },    { "Plane", 3, false },    { "5", 3, true },
    { "Hangman", 4, false }, { "Spanish", 4, true },  { "Siblings", 5, false }, { "Snake", 5, true },
    { "8", 6, false },       { "Mushroom", 6, true }, { "Z", 7, false },        { "Tetris", 7, true },
    { "Ear", 8, false },     { "Bomb", 8, true },     { "Bird", 9, false },     { "L", 9, true },
    { "O", 10, false },      { "Star", 10, true }
};

bool isPatternReverse() {
    bool b = al::isHalfProbability();
    if (SettingsMgr::instance()->getSettings()->mWigglerPattern != 0)
        b = mPatternEntries[SettingsMgr::instance()->getSettings()->mWigglerPattern - 1].reverse == 1;
    return b;
}

int getPatternTarget(int a) {
    int r = al::getRandom(a);
    if (SettingsMgr::instance()->getSettings()->mWigglerPattern == 0) return r;
    return mPatternEntries[SettingsMgr::instance()->getSettings()->mWigglerPattern - 1].target;
}

inline void installWigglerHooks() {
    // ptr addr = sail::lookupSymbolFromDb<>("WigglerPatternReverse");
    // ptr offset = addr - ro::getMainModule()->range().start();
    // hk::hook::writeBranchLink(ro::getMainModule(), offset, (void*)isPatternReverse);
    hk::hook::writeBranchLinkAtSym<"WigglerPatternReverse">(isPatternReverse);

    // addr = sail::lookupSymbolFromDb<>("WigglerPatternTarget");
    // offset = addr - ro::getMainModule()->range().start();
    // hk::hook::writeBranchLink(ro::getMainModule(), offset, (void*)getPatternTarget);
    hk::hook::writeBranchLinkAtSym<"WigglerPatternTarget">(getPatternTarget);
}

HkTrampoline allCheckpointsHook = [](TrampolineStatic(), GameDataHolderAccessor acc,
                                     int checkpointIdx) -> bool {
    if (SettingsMgr::instance()->getSettings()->mIsEnableAllCheckpoints) {
        auto checkpointTrans = GameDataFunction::getCheckpointTransInWorld(acc, checkpointIdx);
        if (checkpointTrans.x == 0 && checkpointTrans.y == 0 && checkpointTrans.z == 0) {
            return orig(acc, checkpointIdx);
        }
        return true;
    }
    return orig(acc, checkpointIdx);
};

HkTrampoline noclipHook = [](TrampolineStatic(), PlayerActorHakoniwa* player) -> void {
    static bool wasNoclipOn = false;
    bool isNoclip = Menu::instance()->mIsEnableNoclip;
    if (!isNoclip && wasNoclipOn) player->endDemoPuppetable();
    wasNoclipOn = isNoclip;

    if (!isNoclip) {
        orig(player);
        return;
    }

    if (isNoclip) {
        static float speed = 20.0f;
        static float speedMax = 250.0f;
        static float vspeed = 10.0f;
        static float speedGain = 0.0f;

        sead::Vector3f* playerPos = al::getTransPtr(player);
        sead::Vector3f cameraPos = al::getCameraPos(player, 0);
        sead::Vector2f leftStick = { InputHelper::getLeftStickX(), InputHelper::getLeftStickY() };

        player->startDemoPuppetable();

        float d = sqrt(
            (playerPos->x - cameraPos.x) * (playerPos->x - cameraPos.x) +
            ((playerPos->z - cameraPos.z) * (playerPos->z - cameraPos.z))
        );
        float vx = ((speed + speedGain) / d) * (playerPos->x - cameraPos.x);
        float vz = ((speed + speedGain) / d) * (playerPos->z - cameraPos.z);

        playerPos->x -= leftStick.x * vz;
        playerPos->z += leftStick.x * vx;

        playerPos->x += leftStick.y * vx;
        playerPos->z += leftStick.y * vz;

        if (InputHelper::isHoldX() || InputHelper::isHoldY()) speedGain += 0.5f;
        if (InputHelper::isHoldA() || InputHelper::isHoldB()) speedGain -= 0.5f;
        if (speedGain <= 0.0f) speedGain = 0.0f;
        if (speedGain >= speedMax) speedGain = speedMax;

        if (InputHelper::isHoldZL()) playerPos->y -= (vspeed + speedGain / 3);
        if (InputHelper::isHoldZR()) playerPos->y += (vspeed + speedGain / 3);
    }

    orig(player);
};

static void setMapTargetUpdateNullNerve(al::IUseNerve* user, const al::Nerve* nerve) {
    StageSceneStateStageMap* map;
    __asm("mov %0, x19" : "=r"(map));
    void* mapThingPtr;
    __asm("mov %0, x23" : "=r"(mapThingPtr));

    if (mapThingPtr != nullptr) Menu::instance()->setLatestMapTarget(mapThingPtr);

    // al::setNerve(user, nerve);
    user->getNerveKeeper()->setNerve(nerve);
}

HkTrampoline seedRefreshHook = [](TrampolineStatic(), al::LiveActor* actor, al::PlacementId* id) -> int {
    return SettingsMgr::instance()->getSettings()->mIsEnableFlowerPotRefresh ? 0 : orig(actor, id);
};

HkTrampoline seedRefreshHook2 = [](TrampolineStatic(), al::LiveActor* actor, al::PlacementId* id) -> bool {
    return SettingsMgr::instance()->getSettings()->mIsEnableFlowerPotRefresh ? false : orig(actor, id);
};

// The saved grow level of a pot is set to its max once its moon is got, so a refreshed seed would grow at once. With
// Seeds Refresh, pots keep their grow level (water, Gamaner...) in memory for the current scene only.
struct SeedGrowLevel {
    const al::LiveActor* pot;
    u32 level;
};
static SeedGrowLevel sSeedGrowLevels[64];

static SeedGrowLevel* findSeedGrowLevel(const al::LiveActor* pot) {
    for (int i = 0; i < sSeedGrowLevelNum; i++)
        if (sSeedGrowLevels[i].pot == pot) return &sSeedGrowLevels[i];
    if (sSeedGrowLevelNum >= 64) return nullptr;
    sSeedGrowLevels[sSeedGrowLevelNum] = { pot, 0 };
    return &sSeedGrowLevels[sSeedGrowLevelNum++];
}

// Seeds Growing: a big grow level makes a planted seed grow at once (the pot caps it at its max level)
HkTrampoline seedGetGrowLevelHook = [](TrampolineStatic(), al::LiveActor* actor, al::PlacementId* id) -> u32 {
    auto* s = SettingsMgr::instance()->getSettings();
    if (s->mIsEnableSeedGrowing) return 32;
    if (s->mIsEnableFlowerPotRefresh) {
        SeedGrowLevel* entry = findSeedGrowLevel(actor);
        return entry ? entry->level : 0;
    }
    return orig(actor, id);
};

HkTrampoline seedAddGrowLevelHook = [](TrampolineStatic(), al::LiveActor* actor, al::PlacementId* id, u32 level) -> void {
    if (SettingsMgr::instance()->getSettings()->mIsEnableFlowerPotRefresh) {
        if (SeedGrowLevel* entry = findSeedGrowLevel(actor)) entry->level += level;
        return;
    }
    orig(actor, id, level);
};

HkTrampoline hintPhotoHook = [](TrampolineStatic(), al::LiveActor* actor, char* name) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableHintPhotoSpawn || s->mIsEnableLuigiPixels)) ? true : orig(actor, name);
};

constexpr static const char* worldTravelingStatus[] = { "Init",       "CityWorld0", "WaterfallWorld0",
                                                        "LavaWorld0", "MoonWorld0", "PeachCastleWorld0",
                                                        "Last",       "LastAfter" };

HkTrampoline worldTravelingHook = [](TrampolineStatic(), WorldTravelingNpc* actor) -> const char* {
    // Menu::instance()->mWorldTravelingStatus = worldTravelingHook.orig(actor);
    if (SettingsMgr::instance()->getSettings()->mWorldTravelingStatus != 0) {
        return worldTravelingStatus[SettingsMgr::instance()->getSettings()->mWorldTravelingStatus - 1];
    }
    return orig(actor);
};

HkTrampoline worldTravelingHook2 = [](TrampolineStatic(), WorldTravelingNpc* actor) -> bool {
    // Menu::instance()->mWorldTravelingStatus = worldTravelingHook.orig(actor);
    if (SettingsMgr::instance()->getSettings()->mWorldTravelingStatus != 0) {
        return false;
    }
    return orig(actor);
};

HkTrampoline toadRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && s->mIsEnableRefreshNpc) ? false : orig(actor);
};

HkTrampoline hintToadRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && s->mIsEnableRefreshNpc) ? false : orig(actor);
};

HkTrampoline musicToadRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && s->mIsEnableRefreshNpc) ? false : orig(actor);
};

HkTrampoline peachRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && s->mIsEnableRefreshNpc) ? false : orig(actor);
};

HkTrampoline coinStackRefreshHook = [](TrampolineStatic(), al::LiveActor* actor, al::PlacementId* id,
                                       int* i) -> void {
    if (SettingsMgr::instance()->getSettings()->mIsEnableDisableCoinStackSave) {
        return;
    } else {
        orig(actor, id, i);
    }
};

HkTrampoline kingdomEnterHook = [](TrampolineStatic(), GameProgressData* data, int i) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshKingdomEnter || s->mIsEnableRefreshNpc)) ? false : orig(data, i);
};

class SaveObjInfo {
public:
    void* a;
    bool isOn;
};

bool shardRefreshHook(SaveObjInfo* info) {
    return SettingsMgr::getSettings()->mIsEnableShardRefresh ? false : info->isOn;
}

// ShineChipWatcher creates the shards empty (grey), hides them and skips counting them when the moon made of the
// shards is already got in the save: with Moon Shards Refresh, treat that moon as not got at every one of its checks
extern "C" bool rs_isGotShine(const void* shine) asm("_ZN2rs10isGotShineEPK5Shine");
bool shardWatcherIsGotShine(const void* shine) {
    return !SettingsMgr::getSettings()->mIsEnableShardRefresh && rs_isGotShine(shine);
}

// The shard counter at the top of the screen (StageSceneLayout / ShineChipLayoutParts) is not shown for an
// "empty" watcher, i.e. when its moon is already got
HkTrampoline shardWatcherTypeEmptyHook = [](TrampolineStatic(), const al::IUseSceneObjHolder* holder) -> bool {
    return !SettingsMgr::getSettings()->mIsEnableShardRefresh && orig(holder);
};

int pipeRandomness(int param) {
    // not overridden, as no randomness
    if (param == 0 || param == 1) return al::getRandom(param);
    SettingsMgr* set = SettingsMgr::instance();
    if (set->getSettings()->mIsEnablePipeMazeOverride) {
        if (param == 2) return set->getSettings()->mPipeRando1;
        if (param == 3) return set->getSettings()->mPipeRando2;
        if (param == 5) return set->getSettings()->mPipeRando3;

        Logger::instance()->log(
            Logger::LogType::LogWarn, "Unhandled pipeRandomness with param %d! Defaulting to 0.\n", param
        );
        return 0;
    }
    return al::getRandom(param);
}

inline void installPipeRandoHooks() {
    hk::hook::writeBranchLinkAtSym<"$PipeRando1">(pipeRandomness);
    hk::hook::writeBranchLinkAtSym<"$PipeRando2">(pipeRandomness);
    hk::hook::writeBranchLinkAtSym<"$PipeRando3">(pipeRandomness);
    // hk::hook::writeBranchLinkAtSym<"$PipeRando4">(pipeRandomness);
}

void SettingsHooks::installSettingsHooks() {
    playerStartInfoCtorHook.installAtSym<"PlayerStartInfoCtor">();
    doorWarpInitHook.installAtSym<"DoorWarpInit">();
    hk::hook::writeBranchLinkAtSym<"ShineChipRefresh">(shardRefreshHook);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck1">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck2">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck3">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck4">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck5">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck6">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck7">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck8">(shardWatcherIsGotShine);
#if BTT_V130
    // 1.3 duplicates the moon check at the end of ShineChipWatcher::initAfterPlacement (Check3) on two paths
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck3b">(shardWatcherIsGotShine);
#endif
    shardWatcherTypeEmptyHook.installAtSym<"_ZN2rs34isCurrentShineChipWatcherTypeEmptyEPKN2al18IUseSceneObjHolderE">();

    installDemoHooks();
    installWigglerHooks();
    hk::hook::writeBranchLinkAtSym<"LuigiRareRoll">(luigiRareRoll);
    bombTailControlHook.installAtSym<"_ZN8BombTail7controlEv">();
    fireBlowerControlHook.installAtSym<"_ZN10FireBlower7controlEv">();
    capThrowerControlHook.installAtSym<"_ZN10CapThrower7controlEv">();
    breedaControlHook.installAtSym<"_ZN6Breeda7controlEv">();
    mofumofuInitHook.installAtSym<"_ZN8Mofumofu4initERKN2al13ActorInitInfoE">();
    stageSceneInitHook.installAtSym<"_ZN10StageScene4initERKN2al13SceneInitInfoE">();
    odysseyIsLevelUpHook.installAtSym<"OdysseyIsLevelUpByPay">();
    hk::hook::writeBranchLinkAtSym<"OdysseyGetCurrentShineNum">(odysseyGetCurrentShineNum);
    odysseyDecideWorldMapHook.installAtSym<"_ZN16ShineTowerRocket14decideWorldMapEi">();
    odysseyCancelWorldMapHook.installAtSym<"_ZN16ShineTowerRocket14cancelWorldMapEv">();
    odysseyInitHook.installAtSym<"_ZN16ShineTowerRocket4initERKN2al13ActorInitInfoE">();
    odysseyControlHook.installAtSym<"_ZN16ShineTowerRocket7controlEv">();
    worldMapKillHook.installAtSym<"_ZN23StageSceneStateWorldMap4killEv">();
    koopaShipFlyAwayHook.installAtSym<"_ZN2rs32tryStartKoopaShipDemoHomeFlyAwayEP16ShineTowerRocket">();
    isFindKoopaNextHook.installAtSym<"_ZNK14GameDataHolder15isFindKoopaNextEi">();
    isBossAttackedHomeNextHook.installAtSym<"_ZNK14GameDataHolder22isBossAttackedHomeNextEi">();
    worldMapAppearHook.installAtSym<"_ZN23StageSceneStateWorldMap6appearEv">();
    nextLockedWorldNumSceneHook.installAtSym<"_ZN16GameDataFunction33calcNextLockedWorldNumForWorldMapEPKN2al5SceneE">();
#if !BTT_V130 // 1.3 has no LayoutActor variants (the world map layouts use the Scene ones)
    nextLockedWorldNumLayoutHook.installAtSym<"_ZN16GameDataFunction33calcNextLockedWorldNumForWorldMapEPKN2al11LayoutActorE">();
#endif
    nextLockedWorldIdSceneHook.installAtSym<"_ZN16GameDataFunction32calcNextLockedWorldIdForWorldMapEPKN2al5SceneEi">();
#if !BTT_V130
    nextLockedWorldIdLayoutHook.installAtSym<"_ZN16GameDataFunction32calcNextLockedWorldIdForWorldMapEPKN2al11LayoutActorEi">();
#endif
    unlockWorldNumForWorldMapHook.installAtSym<"_ZN16GameDataFunction28getUnlockWorldNumForWorldMapEPKN2al5SceneE">();
    hk::hook::writeBranchLinkAtSym<"OdysseyInitUnlockCheck1">(odysseyInitIsUnlockedWorld);
    hk::hook::writeBranchLinkAtSym<"OdysseyInitUnlockCheck2">(odysseyInitIsUnlockedWorld);
    isExistHomeHook.installAtSym<"_ZN16GameDataFunction11isExistHomeE22GameDataHolderAccessor">();
    hk::hook::writeBranchLinkAtSym<"StageSceneInitUnlockCheck">(stageSceneInitIsUnlockedWorld);
    odysseyTryLevelUpHook.installAtSym<"_ZN16ShineTowerRocket10tryLevelUpEv">();
    odysseyWaitHook.installAtSym<"_ZN16ShineTowerRocket7exeWaitEv">();
    installPipeRandoHooks();

    ptr addr = sail::lookupSymbolFromDb<>("UpdateMapTarget");
    ptr offset = addr - ro::getMainModule()->range().start();
    hk::hook::writeBranchLink(ro::getMainModule(), offset, (void*)setMapTargetUpdateNullNerve);

    addr = sail::lookupSymbolFromDb<>("MoonRefreshText");
    offset = addr - ro::getMainModule()->range().start();
    ro::getMainModule()->writeRo(offset, "BTT Studio", strlen("BTT Studio") + 1);

    checkCostumeNextHook.installAtSym<"_ZNK25EventFlowNodeCheckCostume9getNextIdEv">();
    checkCostumePairNextHook.installAtSym<"_ZNK29EventFlowNodeCheckCostumePair9getNextIdEv">();
    checkCostumeMissMatchNextHook.installAtSym<"_ZNK38EventFlowNodeCheckCostumeMissMatchPart9getNextIdEv">();
    gateKeeperDeadHook.installAtSym<"_ZNK20GateKeeperDataHolder13isAlreadyDeadEv">();
    gateKeeperStartDemoHook.installAtSym<"_ZNK20GateKeeperDataHolder22isAlreadyShowStartDemoEv">();
    gateKeeperFirstDemoHook.installAtSym<"_ZNK30StateGateKeeperBattleStartDemo11isFirstDemoEv">();
    gateKeeperFirstDemoThunkHook.installAtSym<"_ZThn48_NK30StateGateKeeperBattleStartDemo11isFirstDemoEv">();
    GreyShineRefreshHook
        .installAtSym<"_ZN16GameDataFunction10isGotShineE22GameDataHolderAccessorPK9ShineInfo">();
    ShineRefreshHook.installAtSym<"_ZN16GameDataFunction11setGotShineE20GameDataHolderWriterPK9ShineInfo">();
    stageSceneControlHook.installAtSym<"_ZN10StageScene7controlEv">();
    saveHook.installAtSym<"_ZNK10StageScene12isEnableSaveEv">();
    checkpointWarpHook.installAtSym<"_ZNK9MapLayout22isEnableCheckpointWarpEv">();
    disableMoonLockHook.installAtSym<"_ZNK14GameDataHolder18findUnlockShineNumEPbi">();
    NoDamageHook.installAtSym<"_ZN16GameDataFunction12damagePlayerE20GameDataHolderWriter">();
    warpTextHook
        .installAtSym<"_ZN16GameDataFunction34isAlreadyShowExplainCheckpointFlagE22GameDataHolderAccessor">();
    isEnableShowExplainHook
        .installAtSym<"_ZN16GameDataFunction33isEnableShowExplainCheckpointFlagE22GameDataHolderAccessor">();
    gameDataFileAlreadyShowExplainHook
        .installAtSym<"_ZNK12GameDataFile34isAlreadyShowExplainCheckpointFlagEv">();
    refreshPurpsHook.installAtSym<
        "_ZN16GameDataFunction16isGotCoinCollectE22GameDataHolderAccessorRKN2al13ActorInitInfoE">();
    doorRefreshHook.installAtSym<"_ZN14DoorAreaChange4initERKN2al13ActorInitInfoE">();
    seedRefreshHook.installAtSym<"_ZN2rs17getGrowFlowerTimeEPKN2al9LiveActorEPKNS0_11PlacementIdE">();
    seedRefreshHook2.installAtSym<"_ZN2rs20isUsedGrowFlowerSeedEPKN2al9LiveActorEPKNS0_11PlacementIdE">();
    seedGetGrowLevelHook.installAtSym<"_ZN2rs22getGrowFlowerGrowLevelEPKN2al9LiveActorEPKNS0_11PlacementIdE">();
    seedAddGrowLevelHook.installAtSym<"_ZN2rs22addGrowFlowerGrowLevelEPKN2al9LiveActorEPKNS0_11PlacementIdEj">();
    stageEventDemoInitHook.installAtSym<"_ZN14StageEventDemo4initERKN2al13ActorInitInfoE">();
    isOnSaveObjInfoHook.installAtSym<"_ZN2rs15isOnSaveObjInfoEPK11SaveObjInfo">();
    isFindKoopaHook.installAtSym<"_ZN16GameDataFunction11isFindKoopaE22GameDataHolderAccessor">();
    isCrashHomeHook.installAtSym<"_ZN16GameDataFunction11isCrashHomeE22GameDataHolderAccessor">();
    setMainScenarioNoHook.installAtSym<"_ZN12GameDataFile17setMainScenarioNoEi">();
    changeNextStageHook.installAtSym<"_ZN14GameDataHolder15changeNextStageEPK15ChangeStageInfoi">();
    calcNextScenarioNoHook.installAtSym<"_ZN16GameDataFunction18calcNextScenarioNoE22GameDataHolderAccessor">();
    isBossAttackedHomeHook.installAtSym<"_ZN16GameDataFunction18isBossAttackedHomeE22GameDataHolderAccessor">();
    repairHomeByCrashedBossHook.installAtSym<"_ZN16GameDataFunction23repairHomeByCrashedBossE20GameDataHolderWriter">();
    isRepairHomeHook.installAtSym<"_ZN16GameDataFunction12isRepairHomeE22GameDataHolderAccessor">();
    repairHomeHook.installAtSym<"_ZN16GameDataFunction10repairHomeE20GameDataHolderWriter">();
    cloudToLostWarpHook.installAtSym<"_ZN16GameDataFunction35tryChangeNextStageWithDemoWorldWarpE20GameDataHolderWriterPKc">();
    isPlayScenarioCameraHook.installAtSym<"_ZNK12GameDataFile20isPlayScenarioCameraEPK9QuestInfo">();
    endDemoAndChangeNerveHook.installAtSym<"_ZN10StageScene21endDemoAndChangeNerveEv">();
    stageTalkTryStartHook.installAtSym<"_ZN19StageTalkDemoNpcCap12tryStartDemoEv">();
    stageTalkDemoInitHook.installAtSym<"_ZN19StageTalkDemoNpcCap4initERKN2al13ActorInitInfoE">();
    scenarioStartCameraHook.installAtSym<"_ZNK14GameDataHolder32isPlayAlreadyScenarioStartCameraEi">();
    talkMessageFlagHook.installAtSym<"_ZN16GameDataFunction23isFlagOnTalkMessageInfoEPKN2al18IUseSceneObjHolderEi">();
    talkMessageFlagOnHook.installAtSym<"_ZN16GameDataFunction21onFlagTalkMessageInfoEPKN2al18IUseSceneObjHolderEi">();
    checkpointFlagHook.installAtSym<"_ZN2rs31setTouchCheckpointFlagToWatcherEP14CheckpointFlag">();
    koopaLv1MovementHook.installAtSym<"_ZN8KoopaLv18movementEv">();
    allCheckpointsHook
        .installAtSym<"_ZN16GameDataFunction22isGotCheckpointInWorldE22GameDataHolderAccessori">();
    noclipHook.installAtSym<"_ZN19PlayerActorHakoniwa8movementEv">();
    hintPhotoHook.installAtSym<"_ZN2rs19checkSavedHintPhotoEPKN2al9LiveActorEPKc">();
    worldTravelingHook.installAtSym<"_ZN16GameDataFunction23getWorldTravelingStatusEPK17WorldTravelingNpc">();
    worldTravelingHook2
        .installAtSym<"_ZN16GameDataFunction27isFirstWorldTravelingStatusEPK17WorldTravelingNpc">();
    toadRefreshHook.installAtSym<"_ZN2rs34isOnFlagKinopioBrigadeNpcFirstTalkEPKN2al9LiveActorE">();
    hintToadRefreshHook.installAtSym<"_ZN2rs24isOnFlagHintNpcFirstTalkEPKN2al9LiveActorE">();
    musicToadRefreshHook.installAtSym<"_ZN2rs19isTalkCollectBgmNpcEPKN2al9LiveActorE">();
    peachRefreshHook.installAtSym<"_ZN2rs25isTalkWorldTravelingPeachEPKN2al9LiveActorE">();
    coinStackRefreshHook.installAtSym<"_ZN2rs13saveCoinStackEPKN2al9LiveActorEPKNS0_11PlacementIdEi">();
    kingdomEnterHook.installAtSym<"_ZNK16GameProgressData16isAlreadyGoWorldEi">();
}
