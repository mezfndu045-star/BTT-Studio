#pragma once

#include "imgui.h"
#include "src/settings/SettingsMgr.h"
#include <cmath>

namespace btt {

constexpr static ImVec4 sInputDisplayColors[] {
    { 0, 0, 0, 0 },         // None
    { 255, 255, 255, 255 }, // White
    { 128, 128, 128, 255 }, // Gray
    { 0, 0, 0, 255 },       // Black
    { 235, 64, 52, 255 },   // Red
    { 52, 58, 235, 255 },   // Blue
    { 235, 210, 52, 255 },  // Yellow
    { 58, 235, 52, 255 },   // Green
    { 235, 152, 52, 255 },  // Orange
    { 113, 52, 235, 255 },  // Purple
    { 225, 52, 235, 255 },  // Pink
    { 52, 235, 235, 255 },  // Light Blue
    { 255, 255, 255, 255 }  // Multicolor
};

constexpr const char* sInputDisplayColorNames[] {
    "None",   "White", "Gray",   "Black",  "Red",  "Blue",
    "Yellow", "Green", "Orange", "Purple", "Pink", "Light Blue",
    "Multicolor"
};

void drawInputDisplay();
void drawInputDisplayP2();

inline ImVec4 getInputDisplayColor(SettingsMgr::InputDisplayColor color, float phaseOffset = 0.0f) {
    if (color == SettingsMgr::InputDisplayColor::Multicolor) {
        float time = (float)ImGui::GetTime();
        float hue = fmodf(time * 0.45f + phaseOffset, 1.0f);
        if (hue < 0.0f) hue += 1.0f;
        float r = 1.0f, g = 1.0f, b = 1.0f;
        ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
        return ImVec4(r * 255.0f, g * 255.0f, b * 255.0f, 255.0f);
    }
    return sInputDisplayColors[int(color)];
}

} // namespace btt
