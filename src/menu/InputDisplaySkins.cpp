#include "menu/InputDisplaySkins.h"

#include "al/Library/Controller/NpadController.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "helpers/fsHelper.h"
#include "menu/InputDisplay.h"
#include "menu/SubAreaImages.h"

namespace btt {

namespace {

    // Picture skins (romfs ImGuiData/Skins/<id>/skin.txt, made by tools/subareas/gen_skins.py from the Periscope and
    // RetroSpy skins): a background, pictures shown while buttons are held, and stick pictures moving with the sticks
    constexpr int cMaxElements = 40;

    struct Element {
        bool isLabel;
        bool isStick;
        bool isTinted;
        char inputs[48];
        char image[8];
        float x, y, w, h;
        float xRange, yRange;
        bool isRight;
    };

    struct Skin {
        char id[32] = {};
        bool isLoaded = false;
        float w = 0, h = 0;
        Element elements[cMaxElements];
        int elementNum = 0;
    };

    Skin sSkin;

    bool load(const char* id) {
        if (sSkin.isLoaded && strcmp(sSkin.id, id) == 0) return true;
        sSkin = Skin();
        snprintf(sSkin.id, sizeof(sSkin.id), "%s", id);
        char path[96];
        snprintf(path, sizeof(path), "ImGuiData/Skins/%s/skin.txt", id);
        FsLoad::Data file = loadRomfsFile(path, nullptr);
        FsHelper::LoadData data = { .path = path, .buffer = file.buffer, .bufSize = file.size };
        sSkin.isLoaded = true;
        if (!data.buffer) return false;
        char* text = (char*)malloc(data.bufSize + 1);
        if (!text) {
            free(data.buffer);
            return false;
        }
        memcpy(text, data.buffer, data.bufSize);
        text[data.bufSize] = '\0';
        free(data.buffer);
        for (char* line = text; line && *line;) {
            char* next = strchr(line, '\n');
            if (next) *next++ = '\0';
            char kind[8] = {}, a[48] = {}, img[8] = {};
            float x = 0, y = 0, w = 0, h = 0, xr = 0, yr = 0;
            if (sscanf(line, "%7s", kind) == 1) {
                if (strcmp(kind, "bg") == 0) {
                    sscanf(line, "%*s %*s %f %f", &sSkin.w, &sSkin.h);
                } else if (sSkin.elementNum < cMaxElements &&
                           (strcmp(kind, "btn") == 0 || strcmp(kind, "btnt") == 0) &&
                           sscanf(line, "%*s %47s %7s %f %f %f %f", a, img, &x, &y, &w, &h) == 6) {
                    Element& e = sSkin.elements[sSkin.elementNum++];
                    e = { false, false, strcmp(kind, "btnt") == 0, {}, {}, x, y, w, h, 0, 0, false };
                    snprintf(e.inputs, sizeof(e.inputs), "%s", a);
                    snprintf(e.image, sizeof(e.image), "%s", img);
                } else if (sSkin.elementNum < cMaxElements && strcmp(kind, "stick") == 0 &&
                           sscanf(line, "%*s %47s %7s %f %f %f %f %f %f", a, img, &x, &y, &w, &h, &xr, &yr) == 8) {
                    Element& e = sSkin.elements[sSkin.elementNum++];
                    e = { false, true, false, {}, {}, x, y, w, h, xr, yr, a[0] == 'R' };
                    snprintf(e.image, sizeof(e.image), "%s", img);
                } else if (sSkin.elementNum < cMaxElements && strcmp(kind, "label") == 0 &&
                           sscanf(line, "%*s %47s %f %f %f", a, &x, &y, &w) == 4) {
                    // Button name written on the skin (centered on x, y, font size w)
                    Element& e = sSkin.elements[sSkin.elementNum++];
                    e = { true, false, false, {}, {}, x, y, w, 0, 0, 0, false };
                    snprintf(e.inputs, sizeof(e.inputs), "%s", a);
                }
            }
            line = next;
        }
        free(text);
        return sSkin.w > 0;
    }

    bool isInput(al::NpadController* pad, const char* name, size_t len) {
        auto is = [&](const char* n) { return strlen(n) == len && strncmp(name, n, len) == 0; };
        const sead::Vector2f l = { pad->mLeftStick.x, pad->mLeftStick.y };
        const sead::Vector2f r = { pad->mRightStick.x, pad->mRightStick.y };
        if (is("a")) return pad->mPadHold.isOnBit(pad->cPadIdx_A);
        if (is("b")) return pad->mPadHold.isOnBit(pad->cPadIdx_B);
        if (is("x")) return pad->mPadHold.isOnBit(pad->cPadIdx_X);
        if (is("y")) return pad->mPadHold.isOnBit(pad->cPadIdx_Y);
        if (is("l")) return pad->mPadHold.isOnBit(pad->cPadIdx_L);
        if (is("r")) return pad->mPadHold.isOnBit(pad->cPadIdx_R);
        if (is("zl")) return pad->mPadHold.isOnBit(pad->cPadIdx_ZL);
        if (is("zr")) return pad->mPadHold.isOnBit(pad->cPadIdx_ZR);
        if (is("plus")) return pad->mPadHold.isOnBit(pad->cPadIdx_Plus);
        if (is("minus")) return pad->mPadHold.isOnBit(pad->cPadIdx_Minus);
        if (is("up")) return pad->mPadHold.isOnBit(pad->cPadIdx_Up);
        if (is("down")) return pad->mPadHold.isOnBit(pad->cPadIdx_Down);
        if (is("left")) return pad->mPadHold.isOnBit(pad->cPadIdx_Left);
        if (is("right")) return pad->mPadHold.isOnBit(pad->cPadIdx_Right);
        if (is("ls")) return pad->mPadHold.isOnBit(pad->cPadIdx_1);
        if (is("rs")) return pad->mPadHold.isOnBit(pad->cPadIdx_2);
        if (is("home")) return pad->mPadHold.isOnBit(pad->cPadIdx_Home);
        if (is("lsu")) return l.y > 0.5f;
        if (is("lsd")) return l.y < -0.5f;
        if (is("lsl")) return l.x < -0.5f;
        if (is("lsr")) return l.x > 0.5f;
        if (is("rsu")) return r.y > 0.5f;
        if (is("rsd")) return r.y < -0.5f;
        if (is("rsl")) return r.x < -0.5f;
        if (is("rsr")) return r.x > 0.5f;
        return false;
    }

    // "a|x": held when any of them is
    bool isAnyInput(al::NpadController* pad, const char* inputs) {
        for (const char* s = inputs; *s;) {
            const char* bar = strchr(s, '|');
            size_t len = bar ? size_t(bar - s) : strlen(s);
            if (isInput(pad, s, len)) return true;
            s += len + (bar ? 1 : 0);
        }
        return false;
    }

    ImTextureID image(const char* name) {
        char path[96];
        snprintf(path, sizeof(path), "ImGuiData/Skins/%s/%s.astc", sSkin.id, name);
        return getRomfsImage(path);
    }

} // namespace

bool drawSkinInputDisplay(const char* skinId, al::NpadController* pad, const ImVec2& center, float maxW, float maxH,
                          ImVec2* outMin, ImVec2* outMax) {
    if (!load(skinId) || !pad) return false;
    float s = maxW / sSkin.w;
    if (sSkin.h * s > maxH) s = maxH / sSkin.h;
    const ImVec2 tl(center.x - sSkin.w * s / 2, center.y - sSkin.h * s / 2);
    if (outMin) *outMin = tl;
    if (outMax) *outMax = ImVec2(tl.x + sSkin.w * s, tl.y + sSkin.h * s);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (ImTextureID bg = image("bg")) dl->AddImage(bg, tl, ImVec2(tl.x + sSkin.w * s, tl.y + sSkin.h * s));

    const SettingsMgr::Settings* set = SettingsMgr::getSettings();
    ImVec4 pc = getInputDisplayColor(set->mInputDisplayButtonPressedColor, 0.35f);
    const ImU32 tint = IM_COL32(u8(pc.x), u8(pc.y), u8(pc.z), 225);

    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < sSkin.elementNum; i++) {
            const Element& e = sSkin.elements[i];
            if (e.isLabel || e.isStick != (pass == 0)) continue;
            float x = e.x, y = e.y;
            if (e.isStick) {
                const auto& v = e.isRight ? pad->mRightStick : pad->mLeftStick;
                x += v.x * e.xRange;
                y -= v.y * e.yRange;
            } else if (!isAnyInput(pad, e.inputs)) {
                continue;
            }
            ImTextureID tex = image(e.image);
            if (!tex) continue;
            ImVec2 a(tl.x + x * s, tl.y + y * s), b(tl.x + (x + e.w) * s, tl.y + (y + e.h) * s);
            dl->AddImage(tex, a, b, ImVec2(0, 0), ImVec2(1, 1), e.isTinted ? tint : IM_COL32_WHITE);
        }
    }

    // Button names, on top of everything
    if (ImFont* font = ImGui::GetFont()) {
        for (int i = 0; i < sSkin.elementNum; i++) {
            const Element& e = sSkin.elements[i];
            if (!e.isLabel) continue;
            float size = e.w * s < 11.0f ? 11.0f : e.w * s;
            ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, e.inputs);
            ImVec2 at(tl.x + e.x * s - ts.x / 2, tl.y + e.y * s - ts.y / 2);
            dl->AddText(font, size, ImVec2(at.x + 1.5f, at.y + 1.5f), IM_COL32(0, 0, 0, 200), e.inputs);
            dl->AddText(font, size, at, IM_COL32(255, 255, 255, 235), e.inputs);
        }
    }
    return true;
}

} // namespace btt
