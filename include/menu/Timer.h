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

    // Splits (Timer > Splits): time of each split done in this run (ms since the start), and the split being run
    static constexpr int cMaxSplits = 30;
    s64 mSplitMs[cMaxSplits] = {};
    int mSplitIndex = 0;

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
    // The current split is done (End Type event with Splits on): next split, or the end of the run
    void split();

    void draw();
    void drawSplits();

    static Timer* sInstance;
};

} // namespace btt
