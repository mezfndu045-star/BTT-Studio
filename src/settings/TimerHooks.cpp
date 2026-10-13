#include "settings/TimerHooks.h"

#include "hk/hook/Trampoline.h"

#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveKeeper.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Scene/StageScene.h"

#include "menu/Timer.h"

using namespace hk;
using namespace btt;

class ShineCounter : public al::LiveActor {};

static void shineGrabHooks(al::LiveActor* shine) {
    al::invalidateClipping(shine);
    Timer::sInstance->event(TimerHookType::ShineGrab);
}

HkTrampoline shineTickHook = [](TrampolineStatic(), ShineCounter* thisPtr) -> void {
    if (thisPtr->getNerveKeeper()->getCurrentStep() == 1) {
        Timer::sInstance->event(TimerHookType::ShineTick);
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
// (ShineTowerRocket::exeGoToWorldMapWithFade closes its wipe at +0x268, then opens the world map). Repairs and
// the attacked Odyssey skip that message and wait on a black screen in exeDemoUpLevelWaitFade instead.
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
HkTrampoline odysseyUpLevelWaitFadeHook = [](TrampolineStatic(), al::LiveActor* rocket) -> void {
    if (al::isFirstStep(rocket)) Timer::sInstance->event(TimerHookType::OdysseyPowerUp);
    orig(rocket);
};

void TimerHooks::installTimerHooks() {
    shineTickHook.installAtSym<"_ZN12ShineCounter16exeShineCountAddEv">();
    shineGrabHook.installAtSym<"_ZN10StageScene15exeDemoShineGetEv">();
    odysseyPowerUpMessageHook.installAtSym<"_ZN16ShineTowerRocket27exeDemoInformPowerUpMessageEv">();
    odysseyUpLevelWaitFadeHook.installAtSym<"_ZN16ShineTowerRocket22exeDemoUpLevelWaitFadeEv">();
    odysseyGoToWorldMapWithFadeHook.installAtSym<"_ZN16ShineTowerRocket23exeGoToWorldMapWithFadeEv">();
}
