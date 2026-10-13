#pragma once

#include "sead/basis/seadTypes.h"

namespace btt {

enum class TimerHookType : u8 {
    None,
    ShineGrab,
    ShineTick,
    GameStart,
    EscapeWire,
    CaptureGlow,
    OdysseyPowerUp,
    Capture,
    MoonGet,
    BlackScreen,
    WhiteScreen,
    TwoMoons,
    Damage,
    TwoMoonsTick,
};

class Timer {
public:
    int mFrameTimer = 0;
    s64 mStartTick = 0;
    s64 mEndTick = 0;
    bool mIsRunning = false;
    s64 mShowSplitTick = 0;
    int mFrames = 0;
    // Moons got since the timer last started, for the "2 Moons" type
    int mMoonCount = 0;
    // Moon counter ticks since the timer last started, for the "2 Moons Tick" type
    int mMoonTickCount = 0;
    // Start Type events seen since the timer last started (or was reset), for the Start Count setting
    int mStartEventCount = 0;
    // End Type events seen while the timer runs, for the End Count setting
    int mEndEventCount = 0;

public:
    Timer();

    void start();
    void stop();
    void reset();
    void resetEventCounts();
    void event(TimerHookType type);
    void showSplit();
    void onMoonGet();
    void onMoonTick();

    void draw();

    static Timer* sInstance;
};

} // namespace btt
