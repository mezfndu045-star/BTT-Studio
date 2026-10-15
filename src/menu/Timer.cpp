#include "menu/Timer.h"

#include "nn/os.h"

#include <cstdio>

#include "imgui.h"
#include "menu/Lang.h"
#include "menu/Menu.h"
#include "src/settings/SettingsMgr.h"

#include <cstring>

namespace btt {

Timer* Timer::sInstance = nullptr;

namespace {

    s64 nowMs(s64 startTick) {
        s64 freq = nn::os::GetSystemTickFrequency().value;
        s64 t = nn::os::GetSystemTick().value - startTick;
        return t / freq * 1000 + (t % freq) * 1000 / freq;
    }

    int splitCount() {
        int n = SettingsMgr::getSettings()->mSplitCount;
        return n < 2 ? 2 : n > Timer::cMaxSplits ? Timer::cMaxSplits : n;
    }

    // "1:02.345" or "02.345"
    void formatMs(char* out, int size, s64 ms) {
        s64 s = ms / 1000, m = s / 60;
        if (m > 0) snprintf(out, size, "%ld:%02ld.%03ld", m, s % 60, ms % 1000);
        else snprintf(out, size, "%02ld.%03ld", s % 60, ms % 1000);
    }

} // namespace

void Timer::split() {
    if (!mIsRunning) return;
    int count = splitCount();
    if (mSplitIndex >= count) return;
    mSplitMs[mSplitIndex++] = nowMs(mStartTick);
    if (mSplitIndex >= count) stop();
}

Timer::Timer() {
    sInstance = this;
}

void Timer::start() {
    mStartTick = nn::os::GetSystemTick().value;
    mIsRunning = true;
    mFrames = 0;
    mMoonCount = 0;
    mMoonTickCount = 0;
    mStartEventCount = 0;
    mEndEventCount = 0;
    mSplitIndex = 0;
    memset(mSplitMs, 0, sizeof(mSplitMs));
}

void Timer::resetEventCounts() {
    mStartEventCount = 0;
    mEndEventCount = 0;
}

// Counts one event toward a Start Count / End Count setting, true once the count is reached
static bool countEvent(int* count, u8 needed) {
    if (++*count < (needed ? needed : 1)) return false;
    *count = 0;
    return true;
}

// Moon Get fires on every moon, 2 Moons on every second moon since the timer last started
void Timer::onMoonGet() {
    event(TimerHookType::MoonGet);
    if (++mMoonCount >= 2) {
        mMoonCount = 0;
        event(TimerHookType::TwoMoons);
    }
}

// Shine Tick fires each time the moon counter goes up, 2 Moons Tick on every second tick since the timer last started
void Timer::onMoonTick() {
    event(TimerHookType::ShineTick);
    if (++mMoonTickCount >= 2) {
        mMoonTickCount = 0;
        event(TimerHookType::TwoMoonsTick);
    }
}

void Timer::stop() {
    if (mIsRunning) {
        mEndTick = nn::os::GetSystemTick().value;
        mIsRunning = false;
    }
}

void Timer::reset() {
    auto now = nn::os::GetSystemTick();
    mStartTick = now.value;
    mEndTick = now.value;
    mIsRunning = false;
    mFrames = 0;
    resetEventCounts();
    mSplitIndex = 0;
    memset(mSplitMs, 0, sizeof(mSplitMs));
}

void Timer::event(TimerHookType type) {
    auto* settings = btt::SettingsMgr::instance()->getSettings();
    // With Splits, the split being run can have its own end type (Timer > Edit Split / Split Type)
    TimerHookType endType = settings->mTimerEndType;
    if (settings->mIsEnableSplits && mIsRunning && mSplitIndex < cMaxSplits) {
        int t = settings->mSplitTypes[mSplitIndex];
        if (t > 0 && t <= int(sizeof(Menu::VisibleTimerHookTypes) / sizeof(Menu::VisibleTimerHookTypes[0])))
            endType = Menu::VisibleTimerHookTypes[t - 1];
    }
    bool isStartEvent = settings->mTimerStartType == type;
    bool isEndEvent = endType == type;
    if (!isStartEvent && !isEndEvent) return;

    // Same start and end type: each event stops a running timer (or shows a lap with Timer Split), else starts it
    if (isStartEvent && isEndEvent) {
        if (!mIsRunning) {
            if (countEvent(&mStartEventCount, settings->mTimerStartCount)) start();
        } else if (countEvent(&mEndEventCount, settings->mTimerEndCount)) {
            if (settings->mIsEnableSplits)
                split();
            else if (settings->mIsEnableSegTimerSplit)
                showSplit();
            else
                stop();
        }
        return;
    }
    // The start type restarts the timer from zero, the end type stops it (each after its count of events)
    if (isStartEvent) {
        if (countEvent(&mStartEventCount, settings->mTimerStartCount)) {
            mShowSplitTick = 0;
            start();
        }
    } else if (mIsRunning) {
        if (countEvent(&mEndEventCount, settings->mTimerEndCount)) {
            if (settings->mIsEnableSplits) split();
            else stop();
        }
    }
}

void Timer::showSplit() {
    auto tick = nn::os::GetSystemTick();
    if (!mIsRunning) {
        mStartTick = tick.value;
        mEndTick = tick.value;
    }
    mShowSplitTick = tick.value;
}

// The splits under the timer, one row each: the timer's time when the split ended (the split being run shows the
// running time)
void Timer::drawSplits() {
    int count = splitCount();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetIO().Fonts->Fonts[0];
    constexpr float cSize = 28.f, cRowH = 32.f, cWidth = 300.f;
    ImVec2 pos = btt::SettingsMgr::instance()->getSettings()->mTimerPos;
    pos.y += 62.f;
    dl->AddRectFilled(ImVec2(pos.x - 8, pos.y - 4), ImVec2(pos.x + cWidth, pos.y + count * cRowH + 4), IM_COL32(0, 0, 0, 140), 8.f);
    for (int i = 0; i < count; i++) {
        float y = pos.y + i * cRowH;
        bool isCurrent = mIsRunning && i == mSplitIndex;
        if (isCurrent)
            dl->AddRectFilled(ImVec2(pos.x - 6, y), ImVec2(pos.x + cWidth - 2, y + cRowH - 2), IM_COL32(60, 90, 160, 160), 6.f);
        char text[32];
        snprintf(text, sizeof(text), "%s %d", tr("Split"), i + 1);
        dl->AddText(font, cSize, ImVec2(pos.x, y), IM_COL32(255, 255, 255, 255), text);
        ImU32 col = IM_COL32(255, 255, 255, 255);
        if (i < mSplitIndex) formatMs(text, sizeof(text), mSplitMs[i]);
        else if (isCurrent) {
            formatMs(text, sizeof(text), nowMs(mStartTick));
            col = IM_COL32(170, 200, 255, 255);
        } else {
            snprintf(text, sizeof(text), "-");
            col = IM_COL32(170, 170, 170, 255);
        }
        ImVec2 ts = font->CalcTextSizeA(cSize, FLT_MAX, 0.f, text);
        dl->AddText(font, cSize, ImVec2(pos.x + cWidth - 10 - ts.x, y), col, text);
    }
}

void Timer::draw() {
    if (!btt::SettingsMgr::instance()->getSettings()->mIsEnableSegmentTimer) return;

    mFrames++;

    if (true) {
        char buffer[32] { 0 };

        s64 timerNow = mIsRunning ? nn::os::GetSystemTick().value : mEndTick;
        bool isShowSplit = mShowSplitTick && float(nn::os::GetSystemTick().value - mShowSplitTick) /
                                                     nn::os::GetSystemTickFrequency().value <=
                                                 2;
        if (isShowSplit) timerNow = mShowSplitTick;

        s64 time = timerNow - mStartTick;
        s64 seconds = time / nn::os::GetSystemTickFrequency().value;
        s64 minutes = seconds / 60;
        s64 remainingSeconds = seconds % 60;
        s64 milliseconds =
            (time % nn::os::GetSystemTickFrequency().value) * 1000 / nn::os::GetSystemTickFrequency().value;

        if (time / nn::os::GetSystemTickFrequency().value >= 60)
            snprintf(buffer, 32, "%ld:%02ld.%03ld", minutes, remainingSeconds, milliseconds);
        else
            snprintf(buffer, 32, "%02ld.%03ld", remainingSeconds, milliseconds);

        // Draggable zone with mouse when menu is open
        if (btt::Menu::instance() && btt::Menu::instance()->mIsEnabledMenu) {
            ImVec2 pos = btt::SettingsMgr::instance()->getSettings()->mTimerPos;
            ImVec2 textSize = ImGui::GetIO().Fonts->Fonts[0]->CalcTextSizeA(50, FLT_MAX, 0.f, buffer);
            ImVec2 dragMin = { pos.x - 10, pos.y - 6 };
            ImVec2 dragSize = { textSize.x + 20, textSize.y + 12 };
            ImGui::GetForegroundDrawList()->AddRect(
                dragMin, { dragMin.x + dragSize.x, dragMin.y + dragSize.y }, IM_COL32(255, 255, 255, 45), 10.0f, 0, 1.5f
            );
            ImGui::SetCursorScreenPos(dragMin);
            ImGui::InvisibleButton("##TimerDrag", dragSize);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                btt::SettingsMgr::instance()->getSettings()->mTimerPos.x += delta.x;
                btt::SettingsMgr::instance()->getSettings()->mTimerPos.y += delta.y;
            }
        }

        if (btt::SettingsMgr::instance()->getSettings()->mIsEnableSplits) drawSplits();

        if (isShowSplit)
            ImGui::GetForegroundDrawList()->AddText(
                ImGui::GetIO().Fonts->Fonts[0], 50, btt::SettingsMgr::instance()->getSettings()->mTimerPos,
                IM_COL32(255, 0, 0, mFrames % 20 <= 10 ? 255 : 0), buffer
            );
        else
            ImGui::GetForegroundDrawList()->AddText(
                ImGui::GetIO().Fonts->Fonts[0], 50, btt::SettingsMgr::instance()->getSettings()->mTimerPos,
                IM_COL32(255, 255, 255, 255), buffer
            );
    }
}

} // namespace btt
