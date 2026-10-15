#pragma once

#include "imgui.h"

namespace btt {

// Input Display > Style "Motion": the controller of a player (Pro Controller, or the two Joy-Con) drawn in 3D, turning
// like the real one (nn::hid six-axis sensors), with its buttons, sticks and shoulder buttons. False: nothing drawn
// (no controller / no sensor).
bool drawMotionDisplay(int player, const ImVec2& center, float scale);
// Input Display > Reset Motion: the current orientation of the controllers becomes the front view
void resetMotionDisplay();

} // namespace btt
