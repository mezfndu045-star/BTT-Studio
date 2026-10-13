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
#include "al/Library/Player/PlayerUtil.h"

#include "custom/game/MapObj/DoorAreaChange.h"
#include "custom/game/Scene/StageSceneStateStageMap.h"
#include "game/Item/CoinStackGroup.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/System/GameDataUtil.h"
#include "game/System/PlayerHitPointData.h"

#include "helpers/InputHelper.h"
#include "logger.h"
#include "menu/Menu.h"
#include "menu/Timer.h"
#include "settings/DemoHooks.hpp"
#include "src/settings/SettingsMgr.h"

using namespace hk;
using namespace btt;

class ShineInfo;
class ShineChip;
class GrowFlowerPot;
class CheckpointFlag;

HkTrampoline GreyShineRefreshHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                       const ShineInfo* shineInfo) -> bool {
    return SettingsMgr::instance()->getSettings()->mIsEnableGrayMoonRefresh ? false : orig(writer, shineInfo);
};

HkTrampoline ShineRefreshHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                   const ShineInfo* shineInfo) -> void {
    ptr addr = sail::lookupSymbolFromDb<>("MoonRefreshText");
    ptr offset = addr - ro::getMainModule()->range().start();
    const char* text = Menu::instance()->getMoonRefreshText();
    ro::getMainModule()->writeRo(offset, text, strlen(text) + 1);
    if (!SettingsMgr::instance()->getSettings()->mIsEnableMoonRefresh) orig(writer, shineInfo);
};

// Items of type DotMarioCat roll al::isPercentProbability(10.0f) at init to become
// "ドットキャラクター(レア)" (8-bit Luigi popping out of the ground) instead of a coin.
bool luigiRareRoll(f32 percent) {
    return SettingsMgr::instance()->getSettings()->mIsEnableLuigiPixels || al::isPercentProbability(percent);
}

// Skip Broodals: once a Broodal fight's start cutscene ends, finish the boss through its own
// last-hit code so the game plays the defeat cutscene and spawns the Multi Moon as usual.
extern "C" void BombTail_damageAndSetNerve(al::LiveActor* boss) asm("_ZN8BombTail17damageAndSetNerveEv");
extern "C" void FireBlower_damageOrDie(al::LiveActor* boss) asm("_ZN10FireBlower11damageOrDieEv");

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
extern "C" bool Shine_appearCommon(void* shine) asm("_ZN5Shine12appearCommonEv");
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

// Pots with a grow level kept in memory by Seeds Refresh (see seedGetGrowLevelHook), reset on each scene
static int sSeedGrowLevelNum = 0;

HkTrampoline stageSceneInitHook = [](TrampolineStatic(), StageScene* stageScene, const al::SceneInitInfo& info) -> void {
    sNightMetroShine = nullptr;
    sIsNightMetroShineAppeared = false;
    sSeedGrowLevelNum = 0;
    orig(stageScene, info);
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

// StageSceneStateWorldMap: unlocked kingdom count at +0x104, cursor index at +0x108, kingdom ids at +0x110
HkTrampoline worldMapAppearHook = [](TrampolineStatic(), void* worldMapState) -> void {
    s32 nextWorldId = getOdysseyForcedNextWorldId();
    if (nextWorldId >= 0) sIsOdysseyWorldMapForced = true;
    orig(worldMapState);
    if (nextWorldId < 0) return;
    u8* state = reinterpret_cast<u8*>(worldMapState);
    s32 count = *reinterpret_cast<s32*>(state + 0x104);
    s32* worldIds = *reinterpret_cast<s32**>(state + 0x110);
    for (s32 i = 0; worldIds && i < count; i++)
        if (worldIds[i] == nextWorldId) *reinterpret_cast<s32*>(state + 0x108) = i;
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
            *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + 0x118) = 0;
        sIsOdysseyRedirectNeedsUnlock = false;
    }
    if (sOdysseyRedirectWorldId >= 0) {
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + 0xfc) = sOdysseyRedirectWorldId;
        *reinterpret_cast<s32*>(reinterpret_cast<u8*>(worldMapState) + 0x118) = 0;
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

HkTrampoline warpTextHook = [](TrampolineStatic(), GameDataHolderAccessor accessor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshWarpText || s->mIsEnableRefreshNpc)) ? false : orig(accessor);
};

HkTrampoline isEnableShowExplainHook = [](TrampolineStatic(), GameDataHolderAccessor accessor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshWarpText || s->mIsEnableRefreshNpc)) ? true : orig(accessor);
};

HkTrampoline gameDataFileAlreadyShowExplainHook = [](TrampolineStatic(), void* file) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshWarpText || s->mIsEnableRefreshNpc)) ? false : orig(file);
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
    if (!s || !s->mIsEnableNoCheckpointTouch || s->mIsEnableRefreshWarpText) orig(checkpointFlag);
};

HkTrampoline cloudSkipHook = [](TrampolineStatic(), StageScene* stageScene) -> bool {
    Menu::instance()->noGetPlayer = true;
    static int functionCalls = 0;
    if (!SettingsMgr::instance()->getSettings()->mIsEnableSkipCloud) {
        functionCalls = 0;
        Menu::instance()->noGetPlayer = false;
        return orig(stageScene);
    }
    // When debugging, returning true right away will skip Lost and go straight to (Day!) Metro with a broken
    // Odyssey back in Wooded. Since isDefeatKoopaLv1 is called multiple times, we can return false the first
    // time, then return true the second time, which seems to correctly skip the Bowser fight. This is a very
    // hacky way of skipping the Bowser fight, so if there is some way to deduce from other game variables
    // which function call we're in, that would be ideal. It would eliminate the need to introduce our own
    // variable to keep track of the number of isDefeatKoopaLv1 calls.
    functionCalls++;
    if (functionCalls == 2) {
        functionCalls = 0;
        return true;
    }
    Menu::instance()->noGetPlayer = false;

    return false;
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
    return (s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshWarpText)) ? false : orig(actor);
};

HkTrampoline hintToadRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshWarpText)) ? false : orig(actor);
};

HkTrampoline musicToadRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshWarpText)) ? false : orig(actor);
};

HkTrampoline peachRefreshHook = [](TrampolineStatic(), al::LiveActor* actor) -> bool {
    auto* s = SettingsMgr::instance()->getSettings();
    return (s && (s->mIsEnableRefreshNpc || s->mIsEnableRefreshWarpText)) ? false : orig(actor);
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
    hk::hook::writeBranchLinkAtSym<"ShineChipRefresh">(shardRefreshHook);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck1">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck2">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck3">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck4">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck5">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck6">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck7">(shardWatcherIsGotShine);
    hk::hook::writeBranchLinkAtSym<"ShineChipWatcherGotCheck8">(shardWatcherIsGotShine);
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
    nextLockedWorldNumLayoutHook.installAtSym<"_ZN16GameDataFunction33calcNextLockedWorldNumForWorldMapEPKN2al11LayoutActorE">();
    nextLockedWorldIdSceneHook.installAtSym<"_ZN16GameDataFunction32calcNextLockedWorldIdForWorldMapEPKN2al5SceneEi">();
    nextLockedWorldIdLayoutHook.installAtSym<"_ZN16GameDataFunction32calcNextLockedWorldIdForWorldMapEPKN2al11LayoutActorEi">();
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
    checkpointFlagHook.installAtSym<"_ZN2rs31setTouchCheckpointFlagToWatcherEP14CheckpointFlag">();
    cloudSkipHook.installAtSym<"_ZNK10StageScene16isDefeatKoopaLv1Ev">();
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
