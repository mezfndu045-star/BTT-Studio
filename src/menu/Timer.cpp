#include "menu/Timer.h"

#include "nn/os.h"

#include <cstdio>

#include "imgui.h"
#include "menu/Menu.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

Timer* Timer::sInstance = nullptr;

Timer::Timer() {
    sInstance = this;
}

void Timer::start() {
    mStartTick = nn::os::GetSystemTick().value;
    mIsRunning = true;
    mFrames = 0;
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
}

void Timer::event(TimerHookType type) {
    bool isStartEvent = btt::SettingsMgr::instance()->getSettings()->mTimerStartType == type;
    bool isEndEvent = btt::SettingsMgr::instance()->getSettings()->mTimerEndType == type;
    if (!mIsRunning && isStartEvent)
        start();
    else if (btt::SettingsMgr::instance()->getSettings()->mIsEnableSegTimerSplit)
        showSplit();
    else if (mIsRunning && isEndEvent) {
        stop();
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
