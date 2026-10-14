#pragma once

#include "imgui.h"
#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "menu/Lang.h"

namespace btt {

// Help panel drawn next to the menu for the selected entry (Menu.cpp)
void drawMenuHelpPanel(const ImVec2& at, const char* title, const char* body);

class MenuComponent {
protected:
    bool mIsFocused = false;
    bool mIsHovered = false;
    const char* mHelp = nullptr;

public:
    virtual ~MenuComponent() = default;
    virtual ImVec2 getSize() const = 0;
    virtual void draw(const ImVec2& at) = 0;
    virtual bool canHaveFocus() const { return false; }
    virtual void onAction() {}
    virtual void onLeft() {}
    virtual void onRight() {}
    // A pressed again while the component is focused (after choosing a value)
    virtual void onConfirm() {}
    // Quick wheel: name of the action (on/off options and buttons) and the value an on/off option toggles
    virtual const char* getWheelName() const { return nullptr; }
    virtual bool* getWheelToggle() { return nullptr; }
    // Drawn next to the selected component, right of the menu cards (at = top left corner)
    // Shown next to the menu while the entry is selected: the help text by default
    virtual void drawPreview(const ImVec2& at) {
        if (mHelp) drawMenuHelpPanel(at, tr(getHelpTitle()), tr(mHelp));
    }
    void setHelp(const char* help) { mHelp = help; }
    virtual const char* getHelpTitle() const { return getWheelName(); }

    void setIsFocused(bool f) { mIsFocused = f; }
    void setIsHovered(bool h) { mIsHovered = h; }
};

class BoolMenuComponent : public MenuComponent {
    static const char* boolText(bool v) { return isFrench() ? (v ? "oui" : "non") : (v ? "true" : "false"); }
    bool* mValue;
    const char* mTitle;
    int mFontSize;

public:
    BoolMenuComponent(bool* val, const char* title, int fontSize = 34)
        : mValue(val), mTitle(title), mFontSize(fontSize) {}

    ImVec2 getSize() const override {
        char buf[160];
        snprintf(buf, sizeof(buf), "%s%s%s", tr(mTitle), trColon(), boolText(mValue && *mValue));
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        bool val = mValue ? *mValue : false;
        ImFont* font = ImGui::GetFont();
        ImU32 textCol = val ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 120, 120, 255);
        if (font) {
            char titleBuf[160];
            snprintf(titleBuf, sizeof(titleBuf), "%s%s", tr(mTitle), trColon());
            ImVec2 titleSize = font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f + titleSize.x, at.y + 6.0f), textCol, boolText(val));
        }
    }

    void onAction() override { if (mValue) *mValue = !*mValue; }
    void onLeft() override { if (mValue) *mValue = !*mValue; }
    void onRight() override { if (mValue) *mValue = !*mValue; }
    const char* getWheelName() const override { return mTitle; }
    bool* getWheelToggle() override { return mValue; }
};

class ButtonMenuComponent : public MenuComponent {
    const char* mText;
    int mFontSize;
    std::function<void()> mFunctor;

public:
    ButtonMenuComponent(const char* text, std::function<void()> func, int fontSize = 34)
        : mText(text), mFontSize(fontSize), mFunctor(func) {}

    ImVec2 getSize() const override {
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, tr(mText)) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        ImFont* font = ImGui::GetFont();
        if (font) {
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), tr(mText));
        }
    }

    void onAction() override { if (mFunctor) mFunctor(); }
    const char* getWheelName() const override { return mText; }
};

template <typename T>
class EnumMenuComponent : public MenuComponent {
    T* mValue;
    const char* const* mNames;
    int mCount;
    const char* mTitle;
    int mFontSize;
    std::function<void(T)> mOnChange;

public:
    EnumMenuComponent(T* val, const char* const* names, int count, const char* title, int fontSize = 34, std::function<void(T)> onChange = nullptr)
        : mValue(val), mNames(names), mCount(count), mTitle(title), mFontSize(fontSize), mOnChange(onChange) {}

    bool canHaveFocus() const override { return true; }

    ImVec2 getSize() const override {
        int idx = mValue ? int(*mValue) : 0;
        if (idx < 0) idx = 0;
        if (idx >= mCount) idx = mCount - 1;
        const char* valStr = (mNames && mCount > 0) ? tr(mNames[idx]) : "";
        char buf[192];
        snprintf(buf, sizeof(buf), "%s%s%s", tr(mTitle), trColon(), valStr);
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        int idx = mValue ? int(*mValue) : 0;
        if (idx < 0) idx = 0;
        if (idx >= mCount) idx = mCount - 1;
        const char* valStr = (mNames && mCount > 0) ? tr(mNames[idx]) : "";
        char titleBuf[160];
        snprintf(titleBuf, sizeof(titleBuf), "%s%s", tr(mTitle), trColon());
        ImFont* font = ImGui::GetFont();
        if (font) {
            ImVec2 titleSize = font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), titleBuf);
            ImU32 valCol = mIsFocused ? IM_COL32(255, 215, 0, 255) : IM_COL32(200, 230, 255, 255);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f + titleSize.x, at.y + 6.0f), valCol, valStr);
        }
    }

    void onAction() override { mIsFocused = !mIsFocused; }
    void onLeft() override {
        if (!mValue || mCount <= 0) return;
        int cur = int(*mValue);
        if (cur > 0) *mValue = (T)(cur - 1);
        else *mValue = (T)(mCount - 1);
        if (mOnChange) mOnChange(*mValue);
    }
    void onRight() override {
        if (!mValue || mCount <= 0) return;
        int cur = int(*mValue);
        if (cur < mCount - 1) *mValue = (T)(cur + 1);
        else *mValue = (T)0;
        if (mOnChange) mOnChange(*mValue);
    }
};

template <typename T>
class IntMenuComponent : public MenuComponent {
    T* mValue;
    const char* mTitle;
    T mMin;
    T mMax;
    T mStep;
    int mFontSize;
    std::function<void(T)> mOnChange;

public:
    IntMenuComponent(T* val, const char* title, T minVal, T maxVal, T step = 1, int fontSize = 34, std::function<void(T)> onChange = nullptr)
        : mValue(val), mTitle(title), mMin(minVal), mMax(maxVal), mStep(step), mFontSize(fontSize), mOnChange(onChange) {}

    bool canHaveFocus() const override { return true; }

    ImVec2 getSize() const override {
        char buf[160];
        snprintf(buf, sizeof(buf), "%s%s%d", tr(mTitle), trColon(), mValue ? (int)(*mValue) : 0);
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        char titleBuf[160];
        snprintf(titleBuf, sizeof(titleBuf), "%s%s", tr(mTitle), trColon());
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%d", mValue ? (int)(*mValue) : 0);
        ImFont* font = ImGui::GetFont();
        if (font) {
            ImVec2 titleSize = font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), titleBuf);
            ImU32 valCol = mIsFocused ? IM_COL32(255, 215, 0, 255) : IM_COL32(200, 230, 255, 255);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f + titleSize.x, at.y + 6.0f), valCol, valBuf);
        }
    }

    void onAction() override { mIsFocused = !mIsFocused; }
    void onLeft() override {
        if (mValue) {
            if (*mValue >= mMin + mStep) *mValue -= mStep;
            else *mValue = mMin;
            if (mOnChange) mOnChange(*mValue);
        }
    }
    void onRight() override {
        if (mValue) {
            if (*mValue + mStep <= mMax) *mValue += mStep;
            else *mValue = mMax;
            if (mOnChange) mOnChange(*mValue);
        }
    }
};

} // namespace btt
