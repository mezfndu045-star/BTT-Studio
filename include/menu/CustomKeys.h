#pragma once

#include "menu/MenuComponent.h"

#include <cstdint>
#include <cstdio>

namespace btt {

// Keybinds > "Custom": any combination of buttons for a keybind. A combination is a mask of the 16 controller buttons
// (bit i = nn::hid::NpadButton i: A, B, X, Y, L3, R3, L, R, ZL, ZR, +, -, D-Pad Left / Up / Right / Down), kept in
// SettingsMgr::Settings::mCustomKeys (one per keybind, CustomKeySlot). It fires when all its buttons are held, the
// frame the last one is pressed.
enum CustomKeySlot : int {
    OpenMenu,
    SavePos,
    LoadPos,
    RestartScene,
    KillScene,
    HealMario,
    LifeUp,
    PrevScene,
    WarpLastCp,
    TimerStart,
    TimerEnd,
    TimerReset,
    IncTpIndex,
    DecTpIndex,
    AddCoins,
    DecCoins,
    IncPattern,
    DecPattern,
    CustomKeySlotNum
};

// The keybind of a SettingsMgr::Settings field (mSavePosKey, mHealMarioKey...), -1 if unknown
int customKeySlotOf(const void* settingField);
bool isCustomKeyPressed(int slot);
// "L + ZR + A" (or "-" without buttons)
void formatCustomKey(uint16_t mask, char* out, int size);

// Recording a combination: starts once every button is released, takes every button held together, ends when they
// are all released again. The menu ignores the controller meanwhile (updateCustomKeyCapture returns true).
void startCustomKeyCapture(int slot);
bool isCustomKeyCapturing(int slot = -1);
bool updateCustomKeyCapture();
uint16_t getCustomKey(int slot);

// A keybind entry: the usual choices, and "Custom" (customValue): A on it records a combination
template <typename T>
class KeyBindMenuComponent : public MenuComponent {
    T* mValue;
    const char* const* mNames;
    int mCount;
    const char* mTitle;
    int mCustomValue;
    int mSlot;

    void valueText(char* buf, int size) const {
        int idx = mValue ? int(*mValue) : 0;
        if (idx < 0 || idx >= mCount) idx = 0;
        if (idx != mCustomValue) {
            snprintf(buf, size, "%s", tr(mNames[idx]));
        } else if (isCustomKeyCapturing(mSlot)) {
            snprintf(buf, size, "%s", tr("Press the buttons..."));
        } else {
            char keys[96];
            formatCustomKey(getCustomKey(mSlot), keys, sizeof(keys));
            snprintf(buf, size, "%s (%s)", tr("Custom"), keys);
        }
    }

public:
    KeyBindMenuComponent(T* val, const char* const* names, int count, const char* title, int customValue, int slot)
        : mValue(val), mNames(names), mCount(count), mTitle(title), mCustomValue(customValue), mSlot(slot) {}

    bool canHaveFocus() const override { return true; }

    ImVec2 getSize() const override {
        char val[128], buf[192];
        valueText(val, sizeof(val));
        snprintf(buf, sizeof(buf), "%s%s%s", tr(mTitle), trColon(), val);
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(34, FLT_MAX, 0.f, buf) : ImVec2(200, 34.f);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        char val[128], titleBuf[160];
        valueText(val, sizeof(val));
        snprintf(titleBuf, sizeof(titleBuf), "%s%s", tr(mTitle), trColon());
        ImFont* font = ImGui::GetFont();
        if (!font) return;
        ImVec2 titleSize = font->CalcTextSizeA(34, FLT_MAX, 0.f, titleBuf);
        ImGui::GetForegroundDrawList()->AddText(font, 34.f, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), titleBuf);
        ImU32 valCol = mIsFocused || isCustomKeyCapturing(mSlot) ? IM_COL32(255, 215, 0, 255) : IM_COL32(200, 230, 255, 255);
        ImGui::GetForegroundDrawList()->AddText(font, 34.f, ImVec2(at.x + 12.0f + titleSize.x, at.y + 6.0f), valCol, val);
    }

    void onAction() override { mIsFocused = !mIsFocused; }
    void onLeft() override {
        if (!mValue || mCount <= 0) return;
        int cur = int(*mValue);
        *mValue = (T)(cur > 0 ? cur - 1 : mCount - 1);
    }
    void onRight() override {
        if (!mValue || mCount <= 0) return;
        int cur = int(*mValue);
        *mValue = (T)(cur < mCount - 1 ? cur + 1 : 0);
    }
    void onConfirm() override {
        if (mValue && int(*mValue) == mCustomValue) startCustomKeyCapture(mSlot);
    }
};

} // namespace btt
