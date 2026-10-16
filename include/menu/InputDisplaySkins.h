#pragma once

#include "imgui.h"

namespace al {
class NpadController;
}

namespace btt {

// Input Display > Controller Pattern (Settings::mSkinPattern): designs of the SCUF styles, on any controller with a body
// ("pat <name with _ for spaces> ..." lines of skin.txt, tools/subareas/gen_skin_variants.py)
constexpr const char* cSkinPatternNames[] = { "None", "Carbon", "Camo", "Galaxy", "Nebula", "Supernova", "Star Storm", "Tesla", "Honeycomb", "Honeycomb White", "Apex Legends", "Knights of SCUF", "Night", "Reaper", "Sand Storm", "Utopia" };

// Draws a picture skin (romfs ImGuiData/Skins/<skinId>) fitted in maxW x maxH around center; false if it is missing
bool drawSkinInputDisplay(const char* skinId, al::NpadController* pad, const ImVec2& center, float maxW, float maxH,
                          ImVec2* outMin, ImVec2* outMax);

} // namespace btt
