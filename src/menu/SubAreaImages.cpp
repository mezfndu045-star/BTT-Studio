#include "menu/SubAreaImages.h"

#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/gfx/Nvn.h"
#include "hk/gfx/Texture.h"
#include "nvn/nvn_Cpp.h"
#include "nvn/nvn_CppMethods.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "heap/seadHeap.h"

#include "helpers/fsHelper.h"

namespace btt {

sead::Heap* getHeap();

namespace {

    // Pictures loaded at once: the input display skins (up to ~20 pictures) and the sub area previews
    constexpr int cSlotNum = 40;
    // The GPU can still read a texture drawn in the last frames (3 frames in flight): never free those
    constexpr int cKeepFrames = 6;

    struct Slot {
        char stage[96] = {};
        hk::gfx::Texture* texture = nullptr;
        void* memory = nullptr;
        hk::gfx::TextureHandle handle {};
        int lastUseFrame = -1000;
        bool isMissing = false;
    };

    Slot sSlots[cSlotNum];

    void freeSlot(Slot& slot) {
        if (slot.texture) {
            slot.texture->~Texture();
            getHeap()->free(slot.texture);
        }
        if (slot.memory) getHeap()->free(slot.memory);
        slot = Slot();
    }

    // key = path relative to content:/
    bool loadSlot(Slot& slot, const char* key) {
        const char* stage = key;
        snprintf(slot.stage, sizeof(slot.stage), "%s", key);

        char path[128];
        snprintf(path, sizeof(path), "content:/%s", key);
        FsHelper::LoadData data = { .path = path };
        FsHelper::loadFileFromPath(data, getHeap());
        // loadFileFromPath may fall back on malloc: free() releases both
        if (!data.buffer || data.bufSize <= (long)sizeof(hk::gfx::AstcHeader)) {
            if (data.buffer) free(data.buffer);
            slot.isMissing = true;
            return false;
        }

        void* device = hk::gfx::ImGuiBackendNvn::instance()->getDevice();
        const auto* header = reinterpret_cast<const hk::gfx::AstcHeader*>(data.buffer);
        size_t texSize = data.bufSize - sizeof(hk::gfx::AstcHeader);

        nvn::SamplerBuilder samp;
        samp.SetDefaults()
            .SetMinMagFilter(nvn::MinFilter::LINEAR, nvn::MagFilter::LINEAR)
            .SetWrapMode(nvn::WrapMode::CLAMP, nvn::WrapMode::CLAMP, nvn::WrapMode::CLAMP);
        nvn::TextureBuilder tex;
        // Linear (not sRGB) ASTC: the menu is drawn without sRGB conversion, like the images were encoded (astcenc -cl)
        tex.SetDefaults()
            .SetDevice(reinterpret_cast<nvn::Device*>(device))
            .SetTarget(nvn::TextureTarget::TARGET_2D)
            .SetFormat(header->block_x == 4 ? nvn::Format::RGBA_ASTC_4x4 : nvn::Format::RGBA_ASTC_8x8)
            .SetSize2D(header->getWidth(), header->getHeight());
        // The GPU layout (block linear) can need more memory than the ASTC data: reserve what the texture needs
        size_t storageSize = tex.GetStorageSize();
        size_t poolSize = storageSize > texSize ? storageSize : texSize;
        size_t memSize = hk::gfx::Texture::calcMemorySize(device, poolSize);
        slot.memory = getHeap()->tryAlloc(memSize, 0x1000);
        void* texStorage = getHeap()->tryAlloc(sizeof(hk::gfx::Texture), 8);
        if (!slot.memory || !texStorage) {
            if (texStorage) getHeap()->free(texStorage);
            free(data.buffer);
            freeSlot(slot);
            // Remember the failure instead of trying again every frame
            snprintf(slot.stage, sizeof(slot.stage), "%s", stage);
            slot.isMissing = true;
            return false;
        }

        slot.texture = new (texStorage) hk::gfx::Texture(
            device, &samp, &tex, poolSize, (void*)(uintptr_t(data.buffer) + sizeof(hk::gfx::AstcHeader)), slot.memory
        );
        slot.handle = slot.texture->getTextureHandle();
        free(data.buffer);
        return true;
    }

} // namespace

ImTextureID getSubAreaImage(const char* stage) {
    if (!stage) return 0;
    char key[96];
    snprintf(key, sizeof(key), "ImGuiData/SubAreas/%s.astc", stage);
    return getRomfsImage(key);
}

ImTextureID getRomfsImage(const char* stage) {
    if (!stage || !getHeap()) return 0;
    const int frame = ImGui::GetFrameCount();

    for (Slot& slot : sSlots) {
        if (slot.stage[0] && strcmp(slot.stage, stage) == 0) {
            slot.lastUseFrame = frame;
            return slot.texture ? (ImTextureID)(uintptr_t)&slot.handle : 0;
        }
    }

    // Reuse the slot unused for the longest time, if the GPU is done with it
    Slot* oldest = nullptr;
    for (Slot& slot : sSlots) {
        if (!slot.stage[0]) {
            oldest = &slot;
            break;
        }
        if (frame - slot.lastUseFrame > cKeepFrames && (!oldest || slot.lastUseFrame < oldest->lastUseFrame)) oldest = &slot;
    }
    if (!oldest) return 0;

    freeSlot(*oldest);
    oldest->lastUseFrame = frame;
    if (!loadSlot(*oldest, stage)) return 0;
    return (ImTextureID)(uintptr_t)&oldest->handle;
}

} // namespace btt
