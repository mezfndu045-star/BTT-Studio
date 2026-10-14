#pragma once

#include "imgui.h"

namespace btt {

// Look of the input display (Settings::mInputDisplayStyle, saved as u8: only append new styles)
enum class InputDisplayStyle : unsigned char {
    Classic,
    // Picture skins (romfs ImGuiData/Skins): Periscope's skin (CC BY-SA 4.0) and Minimal, drawn by
    // tools/subareas/gen_drawn_skins.py
    Periscope,
    Minimal,
};

// Folder of the picture skin of each style from Periscope on
constexpr const char* sInputDisplaySkinIds[] {
    "Periscope",
    "Minimal",
};

constexpr const char* sInputDisplayStyleNames[] {
    "Classic",
    "Periscope",
    "Minimal",
};

// Every style but Classic: a picture skin drawn around the input display position
void drawStyledInputDisplay(int port, ImVec2* settingPos, const char* dragId, const char* label);

} // namespace btt
