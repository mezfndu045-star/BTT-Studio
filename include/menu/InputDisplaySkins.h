#pragma once

#include "imgui.h"

namespace al {
class NpadController;
}

namespace btt {

// Draws a picture skin (romfs ImGuiData/Skins/<skinId>) fitted in maxW x maxH around center; false if it is missing
bool drawSkinInputDisplay(const char* skinId, al::NpadController* pad, const ImVec2& center, float maxW, float maxH,
                          ImVec2* outMin, ImVec2* outMax);

} // namespace btt
