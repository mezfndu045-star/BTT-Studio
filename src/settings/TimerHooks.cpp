#include "settings/TimerHooks.h"

#include "hk/hook/Trampoline.h"
#include "hk/sail/detail.h"

#include <cstring>

#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveKeeper.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Scene/StageScene.h"

#include "helpers/InputHelper.h"
#include "menu/Timer.h"
#include "menu/DebugCommands.h"
#include "helpers/getHelper.h"
#include "logger.h"
#include <typeinfo>
#include "al/Library/Nerve/Nerve.h"

#include "nn/os.h"

using namespace hk;
using namespace btt;

class ShineCounter : public al::LiveActor {};

static void shineGrabHooks(al::LiveActor* shine) {
    al::invalidateClipping(shine);
    Timer::sInstance->event(TimerHookType::ShineGrab);
}

HkTrampoline shineTickHook = [](TrampolineStatic(), ShineCounter* thisPtr) -> void {
    if (thisPtr->getNerveKeeper()->getCurrentStep() == 1) {
        Timer::sInstance->onMoonTick();
    }
    orig(thisPtr);
};
HkTrampoline shineGrabHook = [](TrampolineStatic(), StageScene* thisPtr) -> void {
    if (thisPtr->getNerveKeeper()->getCurrentStep() == 1) {
        Timer::sInstance->event(TimerHookType::ShineGrab);
    }
    orig(thisPtr);
};

// Odyssey Power Up: the frame the screen is fully black after the "The Odyssey has powered up" message
// (ShineTowerRocket::exeGoToWorldMapWithFade closes its wipe at +0x268, then opens the world map). Only this black
// screen counts: every power up also goes through the black screen of exeDemoUpLevelWaitFade before the message, and
// firing there too stopped the timer and started it again right away.
extern "C" bool WipeSimple_isCloseEnd(const void* wipe) asm("_ZNK2al10WipeSimple10isCloseEndEv");

static bool sIsOdysseyPowerUpMessageShown = false;

HkTrampoline odysseyPowerUpMessageHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    sIsOdysseyPowerUpMessageShown = true;
    orig(rocket);
};
HkTrampoline odysseyGoToWorldMapWithFadeHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    const void* wipe = *reinterpret_cast<void**>(reinterpret_cast<u8*>(rocket) + 0x268);
    bool isBlackScreen = sIsOdysseyPowerUpMessageShown && !al::isFirstStep(rocket) && wipe &&
                         WipeSimple_isCloseEnd(wipe);
    orig(rocket);
    if (isBlackScreen) {
        sIsOdysseyPowerUpMessageShown = false;
        Timer::sInstance->event(TimerHookType::OdysseyPowerUp);
    }
};

// Capture and Moon Get fire once per event: ignore a repeat of the same type within half a second
static void eventOnce(TimerHookType type, s64* lastTick) {
    s64 now = nn::os::GetSystemTick().value;
    if (*lastTick != 0 && now - *lastTick < nn::os::GetSystemTickFrequency().value / 2) return;
    *lastTick = now;
    Timer::sInstance->event(type);
}

// Capture: the capture start animation, played for every capture (puppetable captures use their own variant)
static s64 sLastCaptureTick = 0;
HkTrampoline hackStartDemoHook = [](TrampolineStatic(), void* hackKeeper, al::LiveActor* hack) -> void {
    eventOnce(TimerHookType::Capture, &sLastCaptureTick);
    orig(hackKeeper, hack);
};
HkTrampoline hackStartDemoPuppetableHook = [](TrampolineStatic(), void* hackKeeper, al::LiveActor* hack) -> void {
    eventOnce(TimerHookType::Capture, &sLastCaptureTick);
    orig(hackKeeper, hack);
};

// Moon Get: the moment Mario touches the moon (Shine::receiveMsg and Shine::getDirect switch it to
// WaitRequestDemo) or a moon is given with its own get demo (getDirectWithDemo)
static s64 sLastMoonGetTick = 0;
static void onMoonGet() {
    s64 now = nn::os::GetSystemTick().value;
    if (sLastMoonGetTick != 0 && now - sLastMoonGetTick < nn::os::GetSystemTickFrequency().value / 2) return;
    sLastMoonGetTick = now;
    Timer::sInstance->onMoonGet();
}
HkTrampoline shineWaitRequestDemoHook = [](TrampolineStatic(), al::LiveActor* shine) -> void {
    if (al::isFirstStep(shine)) onMoonGet();
    orig(shine);
};
HkTrampoline shineGetDirectWithDemoHook = [](TrampolineStatic(), al::LiveActor* shine) -> void {
    onMoonGet();
    orig(shine);
};

// Black Screen / White Screen: the frame a black (FadeBlack, WipeCircle as used by pipes, WipeResultGrand / Main after a
// Multi Moon / story moon) or white (FadeWhite, as
// used by warps) wipe finishes closing over the screen, i.e. al::WipeSimple switches to its CloseEnd nerve. The game
// inlines exeClose / exeCloseEnd into the nerve executors, so watch al::setNerve for that nerve. The wipe colour
// comes from the layout archive given to its constructor.
enum class WipeColor : u8 { Other, Black, White };
struct WipeColorEntry {
    const void* wipe;
    WipeColor color;
    u32 lastUse;
};
// Only the black / white wipes. Every scene creates its own wipes, while the ones used for stage changes belong to the
// sequence and are created once at boot: a full table drops the least recently used entry (not the oldest one, which
// would drop the sequence's wipes after a few stage loads and miss every black / white screen from then on).
static WipeColorEntry sWipeColors[128] = {};
static u32 sWipeUse = 0;

static void registerWipeColor(const void* wipe, const char* archiveName) {
    WipeColor color = WipeColor::Other;
    // Black: fades, pipes (WipeCircle), and the end of a Multi Moon / story moon get (WipeResultGrand / WipeResultMain,
    // StageSceneStateGetShineGrand / GetShineMain)
    if (archiveName && (strcmp(archiveName, "FadeBlack") == 0 || strcmp(archiveName, "WipeCircle") == 0 ||
                        strcmp(archiveName, "WipeResultGrand") == 0 || strcmp(archiveName, "WipeResultMain") == 0))
        color = WipeColor::Black;
    else if (archiveName && strcmp(archiveName, "FadeWhite") == 0) color = WipeColor::White;
    WipeColorEntry* slot = nullptr;
    for (auto& entry : sWipeColors) {
        if (entry.wipe == wipe) {
            slot = &entry;  // a new wipe at the address of a deleted one
            break;
        }
    }
    if (!slot) {
        if (color == WipeColor::Other) return;
        slot = &sWipeColors[0];
        for (auto& entry : sWipeColors) {
            if (!entry.wipe) {
                slot = &entry;
                break;
            }
            if (entry.lastUse < slot->lastUse) slot = &entry;
        }
    }
    *slot = { color == WipeColor::Other ? nullptr : wipe, color, ++sWipeUse };
}

static WipeColor findWipeColor(const void* wipe) {
    for (auto& entry : sWipeColors) {
        if (entry.wipe == wipe) {
            entry.lastUse = ++sWipeUse;
            return entry.color;
        }
    }
    return WipeColor::Other;
}

HkTrampoline wipeSimpleCtorC1Hook = [](TrampolineStatic(), void* wipe, const char* name, const char* archiveName,
                                       const al::LayoutInitInfo& info, const char* suffix) -> void {
    registerWipeColor(wipe, archiveName);
    orig(wipe, name, archiveName, info, suffix);
};
// Base object constructor: takes the VTT right after `this` (al::WipeSimple uses virtual inheritance)
HkTrampoline wipeSimpleCtorC2Hook = [](TrampolineStatic(), void* wipe, const void* vtt, const char* name,
                                       const char* archiveName, const al::LayoutInitInfo& info,
                                       const char* suffix) -> void {
    registerWipeColor(wipe, archiveName);
    orig(wipe, vtt, name, archiveName, info, suffix);
};

extern "C" bool WipeSimple_isCloseEnd(const void* wipe) asm("_ZNK2al10WipeSimple10isCloseEndEv");

static s64 sLastBlackScreenTick = 0;
static s64 sLastWhiteScreenTick = 0;
static void onWipeClosed(const void* wipe) {
    WipeColor color = findWipeColor(wipe);
    if (color == WipeColor::Black) eventOnce(TimerHookType::BlackScreen, &sLastBlackScreenTick);
    else if (color == WipeColor::White) eventOnce(TimerHookType::WhiteScreen, &sLastWhiteScreenTick);
}
// Press Start (TimerHookType::GameStart): A is pressed on the game mode screen of a new game ("Start" / "Start in
// Assist Mode"), i.e. SceneStateSelectMode leaves its Wait nerve for its Decide nerve. Watched in the nerve hook below.
namespace btt {
void onStageMiss();  // SettingsHooks.cpp
} // namespace btt

HkTrampoline wipeSetNerveHook = [](TrampolineStatic(), al::IUseNerve* user, const al::Nerve* nerve) -> void {
    static const al::Nerve* sWipeCloseEndNerve =
        reinterpret_cast<const al::Nerve*>(hk::sail::lookupSymbolFromDb<>("WipeSimpleNrvCloseEnd"));
    static const al::Nerve* sSelectModeDecideNerve =
        reinterpret_cast<const al::Nerve*>(hk::sail::lookupSymbolFromDb<>("SceneStateSelectModeNrvDecide"));
    static const al::Nerve* sStageMissNerve =
        reinterpret_cast<const al::Nerve*>(hk::sail::lookupSymbolFromDb<>("StageSceneNrvMiss"));
    static const al::Nerve* sWireKoopaChargeNerve =
        reinterpret_cast<const al::Nerve*>(hk::sail::lookupSymbolFromDb<>("ElectricWireKoopaNrvInCharge"));
    orig(user, nerve);
    if (btt::isDebugTrace() && nerve) {
        StageScene* scene = helpers::tryGetStageScene(helpers::tryGetHakoniwaSequence());
        if (scene && static_cast<const void*>(user) == static_cast<const void*>(static_cast<al::IUseNerve*>(scene)))
            btt::dbgLog("DBG: StageScene nerve %s", typeid(*nerve).name());
    }
    // Mario died (StageScene Miss): the stage reloaded after it is not a new start (SettingsHooks.cpp)
    if (nerve == sStageMissNerve) btt::onStageMiss();
    // al::WipeSimple's IUseNerve base is at offset 0, so `user` is the wipe itself
    if (nerve == sWipeCloseEndNerve) onWipeClosed(user);
    else if (nerve == sSelectModeDecideNerve && Timer::sInstance) Timer::sInstance->event(TimerHookType::GameStart);
    // Escape Wire: in the Moon Kingdom escape, captured Bowser is in the final electric wire and swings to leave
    // along it (ElectricWireKoopa: InWait -> InCharge)
    else if (nerve == sWireKoopaChargeNerve && Timer::sInstance) Timer::sInstance->event(TimerHookType::EscapeWire);
};



void TimerHooks::installTimerHooks() {
    shineTickHook.installAtSym<"_ZN12ShineCounter16exeShineCountAddEv">();
    shineGrabHook.installAtSym<"_ZN10StageScene15exeDemoShineGetEv">();
    odysseyPowerUpMessageHook.installAtSym<"_ZN16ShineTowerRocket27exeDemoInformPowerUpMessageEv">();
    odysseyGoToWorldMapWithFadeHook.installAtSym<"_ZN16ShineTowerRocket23exeGoToWorldMapWithFadeEv">();
    hackStartDemoHook.installAtSym<"_ZN16PlayerHackKeeper18startHackStartDemoEPN2al9LiveActorE">();
    hackStartDemoPuppetableHook.installAtSym<"_ZN16PlayerHackKeeper28startHackStartDemoPuppetableEPN2al9LiveActorE">();
    shineWaitRequestDemoHook.installAtSym<"_ZN5Shine18exeWaitRequestDemoEv">();
    shineGetDirectWithDemoHook.installAtSym<"_ZN5Shine17getDirectWithDemoEv">();
    wipeSimpleCtorC1Hook.installAtSym<"_ZN2al10WipeSimpleC1EPKcS2_RKNS_14LayoutInitInfoES2_">();
#if !BTT_V130
    // 1.3 has no separate base object constructor (it was never called in 1.0 either)
    wipeSimpleCtorC2Hook.installAtSym<"_ZN2al10WipeSimpleC2EPKcS2_RKNS_14LayoutInitInfoES2_">();
#endif
    wipeSetNerveHook.installAtSym<"_ZN2al8setNerveEPNS_9IUseNerveEPKNS_5NerveE">();
}
