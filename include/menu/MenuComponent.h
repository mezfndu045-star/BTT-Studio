#pragma once

#include "imgui.h"
#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace btt {

class MenuComponent {
protected:
    bool mIsFocused = false;
    bool mIsHovered = false;

public:
    virtual ~MenuComponent() = default;
    virtual ImVec2 getSize() const = 0;
    virtual void draw(const ImVec2& at) = 0;
    virtual bool canHaveFocus() const { return false; }
    virtual void onAction() {}
    virtual void onLeft() {}
    virtual void onRight() {}

    void setIsFocused(bool f) { mIsFocused = f; }
    void setIsHovered(bool h) { mIsHovered = h; }
};

class BoolMenuComponent : public MenuComponent {
    bool* mValue;
    const char* mTitle;
    int mFontSize;

public:
    BoolMenuComponent(bool* val, const char* title, int fontSize = 34)
        : mValue(val), mTitle(title), mFontSize(fontSize) {}

    ImVec2 getSize() const override {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: %s", mTitle, (mValue && *mValue) ? "true" : "false");
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        bool val = mValue ? *mValue : false;
        ImFont* font = ImGui::GetFont();
        ImU32 textCol = val ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 120, 120, 255);
        if (font) {
            char titleBuf[128];
            snprintf(titleBuf, sizeof(titleBuf), "%s: ", mTitle);
            ImVec2 titleSize = font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), titleBuf);
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f + titleSize.x, at.y + 6.0f), textCol, val ? "true" : "false");
        }
    }

    void onAction() override { if (mValue) *mValue = !*mValue; }
    void onLeft() override { if (mValue) *mValue = !*mValue; }
    void onRight() override { if (mValue) *mValue = !*mValue; }
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
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, mText) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        ImFont* font = ImGui::GetFont();
        if (font) {
            ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12.0f, at.y + 6.0f), IM_COL32(255, 255, 255, 255), mText);
        }
    }

    void onAction() override { if (mFunctor) mFunctor(); }
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
        const char* valStr = (mNames && mCount > 0) ? mNames[idx] : "";
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: %s", mTitle, valStr);
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        int idx = mValue ? int(*mValue) : 0;
        if (idx < 0) idx = 0;
        if (idx >= mCount) idx = mCount - 1;
        const char* valStr = (mNames && mCount > 0) ? mNames[idx] : "";
        char titleBuf[128];
        snprintf(titleBuf, sizeof(titleBuf), "%s: ", mTitle);
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
        char buf[128];
        snprintf(buf, sizeof(buf), "%s: %d", mTitle, mValue ? (int)(*mValue) : 0);
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(200, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        char titleBuf[128];
        snprintf(titleBuf, sizeof(titleBuf), "%s: ", mTitle);
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
