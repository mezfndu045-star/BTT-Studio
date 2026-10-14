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
#include "nn/fs.h" // IWYU pragma: keep

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

    // The romfs images that compress well (input display skins) are stored as Yaz0 (the SZS format): decompress them
    // into a malloc buffer, replacing the file data. Others stay plain ASTC.
    bool tryDecompressYaz0(FsHelper::LoadData& data) {
        const u8* src = static_cast<const u8*>(data.buffer);
        if (data.bufSize < 16 || memcmp(src, "Yaz0", 4) != 0) return true;
        u32 size = u32(src[4]) << 24 | u32(src[5]) << 16 | u32(src[6]) << 8 | u32(src[7]);
        u8* dst = static_cast<u8*>(malloc(size));
        if (!dst) return false;
        const u8* end = src + data.bufSize;
        const u8* in = src + 16;
        u32 out = 0;
        while (out < size && in < end) {
            u8 code = *in++;
            for (int bit = 0; bit < 8 && out < size && in < end; bit++, code <<= 1) {
                if (code & 0x80) {
                    dst[out++] = *in++;
                    continue;
                }
                if (in + 2 > end) break;
                u32 dist = ((in[0] & 0xf) << 8 | in[1]) + 1;
                u32 n = in[0] >> 4;
                in += 2;
                if (n == 0) {
                    if (in >= end) break;
                    n = *in++ + 0x12;
                } else {
                    n += 2;
                }
                if (dist > out) {
                    free(dst);
                    return false;
                }
                for (; n > 0 && out < size; n--, out++) dst[out] = dst[out - dist];
            }
        }
        if (out != size) {
            free(dst);
            return false;
        }
        free(data.buffer);
        data.buffer = dst;
        data.bufSize = size;
        return true;
    }

} // namespace

// Pack of a skin: "BTTP", u32 count, count x { char name[24]; u32 offset; u32 size; }, then the files
FsLoad::Data loadRomfsFile(const char* path, void* heap) {
    FsLoad::Data out;
    char full[160];
    snprintf(full, sizeof(full), "content:/%s", path);
    if (FsHelper::isFileExist(full)) {
        FsHelper::LoadData data = { .path = full };
        FsHelper::loadFileFromPath(data, static_cast<sead::Heap*>(heap));
        out.buffer = data.buffer;
        out.size = data.bufSize;
        return out;
    }
    constexpr const char cSkins[] = "ImGuiData/Skins/";
    if (strncmp(path, cSkins, sizeof(cSkins) - 1) != 0) return out;
    const char* id = path + sizeof(cSkins) - 1;
    const char* slash = strchr(id, '/');
    if (!slash || slash - id > 40) return out;
    const char* name = slash + 1;
    char packPath[96];
    snprintf(packPath, sizeof(packPath), "content:/%s%.*s.pack", cSkins, int(slash - id), id);
    nn::fs::FileHandle handle;
    if (nn::fs::OpenFile(&handle, packPath, nn::fs::OpenMode_Read).IsFailure()) return out;
    u32 head[2] = {};
    if (nn::fs::ReadFile(handle, 0, head, sizeof(head)).IsSuccess() && memcmp(head, "BTTP", 4) == 0 && head[1] < 256) {
        struct Entry {
            char name[24];
            u32 offset;
            u32 size;
        };
        Entry entries[256];
        if (nn::fs::ReadFile(handle, sizeof(head), entries, head[1] * sizeof(Entry)).IsSuccess()) {
            for (u32 i = 0; i < head[1]; i++) {
                if (strncmp(entries[i].name, name, sizeof(entries[i].name)) != 0) continue;
                out.buffer = malloc(entries[i].size);
                if (out.buffer && nn::fs::ReadFile(handle, entries[i].offset, out.buffer, entries[i].size).IsSuccess()) {
                    out.size = entries[i].size;
                } else if (out.buffer) {
                    free(out.buffer);
                    out.buffer = nullptr;
                }
                break;
            }
        }
    }
    nn::fs::CloseFile(handle);
    return out;
}

namespace {

    // key = path relative to content:/
    bool loadSlot(Slot& slot, const char* key) {
        const char* stage = key;
        snprintf(slot.stage, sizeof(slot.stage), "%s", key);

        FsLoad::Data file = loadRomfsFile(key, getHeap());
        FsHelper::LoadData data = { .path = key, .buffer = file.buffer, .bufSize = file.size };
        // An image identical to another one (input display skins sharing pictures) is a "REF:<path>" file: load that one
        if (data.buffer && data.bufSize > 4 && data.bufSize < 120 && memcmp(data.buffer, "REF:", 4) == 0) {
            char refPath[128];
            int len = int(data.bufSize) - 4;
            while (len > 0 && (static_cast<const char*>(data.buffer)[4 + len - 1] <= ' ')) len--;
            snprintf(refPath, sizeof(refPath), "%.*s", len, static_cast<const char*>(data.buffer) + 4);
            free(data.buffer);
            file = loadRomfsFile(refPath, getHeap());
            data.buffer = file.buffer;
            data.bufSize = file.size;
        }
        if (data.buffer && !tryDecompressYaz0(data)) {
            free(data.buffer);
            data.buffer = nullptr;
        }
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
            .SetFormat(header->block_x == 4   ? nvn::Format::RGBA_ASTC_4x4
                       : header->block_x == 6 ? nvn::Format::RGBA_ASTC_6x6
                                              : nvn::Format::RGBA_ASTC_8x8)
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
