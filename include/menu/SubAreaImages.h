#pragma once

#include "imgui.h"

namespace btt {

// Preview image of a sub area (romfs ImGuiData/SubAreas/<stage>.astc), loaded the first time it is asked for.
// Returns 0 while it is not available (missing file, or no free slot this frame).
ImTextureID getSubAreaImage(const char* stage);
// Any ASTC picture of the romfs (path relative to content:/, e.g. "ImGuiData/Skins/NES/bg.astc"), same cache
ImTextureID getRomfsImage(const char* path);

namespace FsLoad {
struct Data {
    void* buffer = nullptr;  // malloc'ed (or from heap, see FsHelper::loadFileFromPath): release with free()
    long size = 0;
};
} // namespace FsLoad
// A romfs file (path relative to content:/). The input display skins are packed: "ImGuiData/Skins/<id>/<name>" is
// read from "ImGuiData/Skins/<id>.pack" when there is no such file (tools/subareas/pack_skins.py). heap may be null.
FsLoad::Data loadRomfsFile(const char* path, void* heap);

} // namespace btt
