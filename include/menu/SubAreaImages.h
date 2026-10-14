#pragma once

#include "imgui.h"

namespace btt {

// Preview image of a sub area (romfs ImGuiData/SubAreas/<stage>.astc), loaded the first time it is asked for.
// Returns 0 while it is not available (missing file, or no free slot this frame).
ImTextureID getSubAreaImage(const char* stage);
// Any ASTC picture of the romfs (path relative to content:/, e.g. "ImGuiData/Skins/NES/bg.astc"), same cache
ImTextureID getRomfsImage(const char* path);

} // namespace btt
