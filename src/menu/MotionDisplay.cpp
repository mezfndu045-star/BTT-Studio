#include "menu/MotionDisplay.h"

#include <algorithm>
#include <cmath>
#include <nn/hid.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"
#include "sead/controller/seadControllerMgr.h"

#include "menu/InputDisplay.h"
#include "menu/MotionModels.h"
#include "src/settings/SettingsMgr.h"

namespace nn::hid {
int GetSixAxisSensorHandles(SixAxisSensorHandle* handles, int count, const u32& npadId, NpadStyleSet style);
void StartSixAxisSensor(const SixAxisSensorHandle& handle);
void GetSixAxisSensorState(SixAxisSensorState* state, const SixAxisSensorHandle& handle);
} // namespace nn::hid

namespace btt {

namespace {

struct Vec3 {
    float x, y, z;
};

// One six-axis sensor of a player: its handle and the orientation taken as the front view (Reset Motion)
struct Sensor {
    nn::hid::SixAxisSensorHandle handle;
    float ref[3][3];
    bool hasRef;
};

struct PlayerMotion {
    int npadId = -1;
    int style = -1;
    int count = 0;
    Sensor sensors[2];
};

PlayerMotion sPlayers[2];
bool sIsResetRequested[2] = {};
// Used when the chosen controller has no sensor: never read (relativeAxes gives the front view)
Sensor sNoSensor = {};

constexpr float cFocal = 700.f;

// The style of the controller: Pro Controller, handheld, two Joy-Con, one Joy-Con (-1: none / not supported)
int styleOf(const nn::hid::NpadStyleSet& set) {
    for (int s = 0; s <= 4; s++)
        if (set.Test(s)) return s;
    return -1;
}

// Input Display > Motion Controller: 0 Auto (the controller in use), 1 Pro Controller, 2 Joy-Con
constexpr int cStyleFullKey = 0, cStyleHandheld = 1, cStyleJoyDual = 2;

// The sensors read: those of the chosen controller (two Joy-Con: held apart, or else attached to the console)
int wantedStyle(int connected) {
    switch (SettingsMgr::getSettings()->mMotionController) {
    case 1: return cStyleFullKey;
    case 2: return connected == cStyleHandheld ? cStyleHandheld : cStyleJoyDual;
    default: return connected;
    }
}

bool updateSensors(int player, PlayerMotion& m, al::NpadController* npad) {
    int npadId = npad->mNpadId;
    if (npadId < 0) return false;
    u32 id = u32(npadId);
    int style = wantedStyle(styleOf(nn::hid::GetNpadStyleSet(id)));
    if (style < 0) return false;
    if (style != m.style || npadId != m.npadId) {
        nn::hid::SixAxisSensorHandle handles[2] = {};
        nn::hid::NpadStyleSet set;
        set.Set(style, true);
        int count = nn::hid::GetSixAxisSensorHandles(handles, 2, id, set);
        m.count = count < 0 ? 0 : count > 2 ? 2 : count;
        for (int i = 0; i < m.count; i++) {
            m.sensors[i].handle = handles[i];
            m.sensors[i].hasRef = false;
            nn::hid::StartSixAxisSensor(handles[i]);
        }
        m.npadId = npadId;
        m.style = style;
    }
    if (sIsResetRequested[player]) {
        for (int i = 0; i < m.count; i++) m.sensors[i].hasRef = false;
        sIsResetRequested[player] = false;
    }
    return true;
}

// The axes of the controller in the front view: rel[i] = axis i (x: right, y: top, z: out of the buttons) seen from
// the reference orientation
bool relativeAxes(Sensor& s, Vec3 rel[3]) {
    rel[0] = { 1, 0, 0 };
    rel[1] = { 0, 1, 0 };
    rel[2] = { 0, 0, 1 };
    if (&s == &sNoSensor) return false;
    nn::hid::SixAxisSensorState state = {};
    nn::hid::GetSixAxisSensorState(&state, s.handle);
    const float(*cur)[3] = state.mDirection.mMtx;
    if (cur[0][0] == 0 && cur[0][1] == 0 && cur[0][2] == 0) return false;
    if (!s.hasRef) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) s.ref[i][j] = cur[i][j];
        s.hasRef = true;
    }
    for (int i = 0; i < 3; i++) {
        float v[3];
        for (int j = 0; j < 3; j++) v[j] = cur[i][0] * s.ref[j][0] + cur[i][1] * s.ref[j][1] + cur[i][2] * s.ref[j][2];
        rel[i] = { v[0], v[1], v[2] };
    }
    return true;
}

// Local point of the controller -> screen (x right, y forward = up on screen, z toward the viewer)
struct Projector {
    ImVec2 center;
    Vec3 axes[3];

    Vec3 rotate(const Vec3& p) const {
        return { p.x * axes[0].x + p.y * axes[1].x + p.z * axes[2].x, p.x * axes[0].y + p.y * axes[1].y + p.z * axes[2].y,
                 p.x * axes[0].z + p.y * axes[1].z + p.z * axes[2].z };
    }
    ImVec2 project(const Vec3& p) const {
        Vec3 r = rotate(p);
        float k = cFocal / (cFocal - r.z);
        return { center.x + r.x * k, center.y - r.y * k };
    }
};

ImU32 shade(ImU32 col, float light) {
    if (light < 0) light = 0;
    if (light > 1) light = 1;
    int r = int(((col >> IM_COL32_R_SHIFT) & 0xff) * light), g = int(((col >> IM_COL32_G_SHIFT) & 0xff) * light),
        b = int(((col >> IM_COL32_B_SHIFT) & 0xff) * light);
    return IM_COL32(r, g, b, (col >> IM_COL32_A_SHIFT) & 0xff);
}

ImU32 toColor(const ImVec4& c) { return IM_COL32(u8(c.x), u8(c.y), u8(c.z), u8(c.w)); }

// Dark text on a light color, white text on a dark one
ImU32 textOn(ImU32 col) {
    int r = (col >> IM_COL32_R_SHIFT) & 0xff, g = (col >> IM_COL32_G_SHIFT) & 0xff, b = (col >> IM_COL32_B_SHIFT) & 0xff;
    return r * 299 + g * 587 + b * 114 > 140000 ? IM_COL32(20, 22, 24, 255) : IM_COL32(245, 245, 245, 255);
}

void drawCenteredText(ImDrawList* dl, const ImVec2& at, float size, ImU32 col, const char* text) {
    ImFont* font = ImGui::GetFont();
    if (!font) return;
    ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.f, text);
    dl->AddText(font, size, ImVec2(at.x - ts.x / 2, at.y - ts.y / 2), col, text);
}

// --- the controller model: the 3D models of HOS-InputDisplay (menu/MotionModels.h), simplified ---------------------

constexpr int cMaxModelVerts = 2048, cMaxModelTris = 4096;

struct DrawTri {
    float depth;
    uint16_t a, b, c;
    bool isFront;
    ImU32 col;
};

Vec3 sModelVerts[cMaxModelVerts];  // rotated, in pixels (y up)
DrawTri sModelTris[cMaxModelTris];

struct ModelColors {
    ImU32 body, dark, face, stick, button, pressed;
};

// The model turned by the projector (unitPx: pixels per model unit), faces turned away dropped, farthest drawn first
template <int VN, int TN, int PN>
void drawMesh(ImDrawList* dl, const Projector& pr, float unitPx, const int16_t (&verts)[VN][3],
              const uint16_t (&tris)[TN][3], const MotionModelPart (&parts)[PN], const ModelColors& colors,
              al::NpadController* npad) {
    static_assert(VN <= cMaxModelVerts && TN <= cMaxModelTris);
    const float k = unitPx / 2048.f;
    for (int i = 0; i < VN; i++) sModelVerts[i] = pr.rotate({ verts[i][0] * k, verts[i][1] * k, verts[i][2] * k });
    int num = 0;
    // Marks of the front (buttons, sticks, their wells): drawn after the body (a big face of the body sorted by its
    // middle could hide them), and only while the front is seen
    const bool isFrontSeen = pr.rotate({ 0, 0, 1 }).z > 0.05f;
    for (const MotionModelPart& part : parts) {
        const bool isFront = part.kind == MotionPartKind::Face || part.kind == MotionPartKind::Button ||
                             part.kind == MotionPartKind::Stick;
        if (isFront && !isFrontSeen) continue;
        bool isHeld = part.padIdx >= 0 && npad->mPadHold.isOnBit(part.padIdx);
        ImU32 col = colors.body;
        switch (part.kind) {
        case MotionPartKind::Dark: col = colors.dark; break;
        case MotionPartKind::Face: col = colors.face; break;
        case MotionPartKind::Stick: col = isHeld ? colors.pressed : colors.stick; break;
        case MotionPartKind::Button:
        case MotionPartKind::Shoulder: col = isHeld ? colors.pressed : colors.button; break;
        default: break;
        }
        // Sticks lean toward where they point
        Vec3 shift = { 0, 0, 0 };
        if (part.kind == MotionPartKind::Stick) {
            const auto& v = part.padIdx == al::NpadController::cPadIdx_1 ? npad->mLeftStick : npad->mRightStick;
            shift = pr.rotate({ v.x * 0.1f * unitPx, v.y * 0.1f * unitPx, 0 });
        }
        for (int t = part.triStart; t < part.triStart + part.triNum; t++) {
            const Vec3& a = sModelVerts[tris[t][0]];
            const Vec3& b = sModelVerts[tris[t][1]];
            const Vec3& c = sModelVerts[tris[t][2]];
            float nx = (b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y);
            float ny = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
            float nz = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (nz <= 0) continue;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            float light = 0.62f + 0.38f * (len > 0 ? nz / len : 0);
            DrawTri& d = sModelTris[num++];
            d.depth = (a.z + b.z + c.z) / 3 + shift.z;
            d.a = tris[t][0];
            d.b = tris[t][1];
            d.c = tris[t][2];
            d.isFront = isFront;
            d.col = shade(col, light);
        }
        if (part.kind == MotionPartKind::Stick && (shift.x != 0 || shift.y != 0)) {
            // Move the stick's vertices once (they belong to it only)
            static bool sIsMoved[cMaxModelVerts];
            for (int t = part.triStart; t < part.triStart + part.triNum; t++)
                for (int j = 0; j < 3; j++) sIsMoved[tris[t][j]] = false;
            for (int t = part.triStart; t < part.triStart + part.triNum; t++)
                for (int j = 0; j < 3; j++) {
                    uint16_t vi = tris[t][j];
                    if (sIsMoved[vi]) continue;
                    sIsMoved[vi] = true;
                    sModelVerts[vi].x += shift.x;
                    sModelVerts[vi].y += shift.y;
                    sModelVerts[vi].z += shift.z;
                }
        }
    }
    std::sort(sModelTris, sModelTris + num, [](const DrawTri& l, const DrawTri& r) {
        return l.isFront != r.isFront ? r.isFront : l.depth < r.depth;
    });
    auto screen = [&](const Vec3& p) {
        float s = cFocal / (cFocal - p.z);
        return ImVec2(pr.center.x + p.x * s, pr.center.y - p.y * s);
    };
    for (int i = 0; i < num; i++) {
        const DrawTri& d = sModelTris[i];
        dl->AddTriangleFilled(screen(sModelVerts[d.a]), screen(sModelVerts[d.b]), screen(sModelVerts[d.c]), d.col);
    }
}

// --- the flat part (like the HOS input display): shoulder buttons, sticks, D-Pad, A B X Y ----------------------------

struct Flat {
    ImDrawList* dl;
    ImVec2 c;
    float s;
    ImU32 idle, pressed, stick, ring;
    al::NpadController* npad;

    ImVec2 at(float x, float y) const { return { c.x + x * s, c.y + y * s }; }
    bool held(int idx) const { return npad->mPadHold.isOnBit(idx); }

    void pill(float x, float y, int idx, const char* text) const {
        ImU32 col = held(idx) ? pressed : idle;
        dl->AddRectFilled(at(x - 80, y - 30), at(x + 80, y + 30), col, 30 * s);
        drawCenteredText(dl, at(x, y), 56 * s, textOn(col), text);
    }
    void button(float x, float y, int idx, const char* text) const {
        ImU32 col = held(idx) ? pressed : idle;
        dl->AddCircleFilled(at(x, y), 50 * s, col, 28);
        drawCenteredText(dl, at(x, y), 56 * s, textOn(col), text);
    }
    void small(float x, float y, int idx, const char* text) const {
        ImU32 col = held(idx) ? pressed : idle;
        dl->AddCircleFilled(at(x, y), 24 * s, col, 20);
        drawCenteredText(dl, at(x, y - 3), 44 * s, textOn(col), text);
    }
    void stickAt(float x, float y, bool isLeft) const {
        const auto& v = isLeft ? npad->mLeftStick : npad->mRightStick;
        int idx = isLeft ? al::NpadController::cPadIdx_1 : al::NpadController::cPadIdx_2;
        dl->AddCircle(at(x, y), 110 * s, ring, 40, 9 * s);
        dl->AddCircleFilled(at(x + v.x * 62, y - v.y * 62), 80 * s, held(idx) ? pressed : stick, 32);
    }
    void dpad(float x, float y) const {
        using P = al::NpadController;
        // The cross, then each pressed arm in the pressed color
        dl->AddRectFilled(at(x - 90, y - 30), at(x + 90, y + 30), idle, 12 * s);
        dl->AddRectFilled(at(x - 30, y - 90), at(x + 30, y + 90), idle, 12 * s);
        if (held(P::cPadIdx_Up)) dl->AddRectFilled(at(x - 30, y - 90), at(x + 30, y - 28), pressed, 12 * s);
        if (held(P::cPadIdx_Down)) dl->AddRectFilled(at(x - 30, y + 28), at(x + 30, y + 90), pressed, 12 * s);
        if (held(P::cPadIdx_Left)) dl->AddRectFilled(at(x - 90, y - 30), at(x - 28, y + 30), pressed, 12 * s);
        if (held(P::cPadIdx_Right)) dl->AddRectFilled(at(x + 28, y - 30), at(x + 90, y + 30), pressed, 12 * s);
    }
};

} // namespace

void resetMotionDisplay() {
    sIsResetRequested[0] = sIsResetRequested[1] = true;
}

bool drawMotionDisplay(int player, const ImVec2& center, float scale) {
    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();
    if (!controllerMgr || player < 0 || player > 1) return false;
    auto* npad = (al::NpadController*)controllerMgr->getController(al::getPlayerControllerPort(player));
    if (!npad) return false;
    PlayerMotion& m = sPlayers[player];
    if (!updateSensors(player, m, npad)) return false;

    using P = al::NpadController;
    SettingsMgr::Settings* set = SettingsMgr::getSettings();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    // Reference layout: 1270 x 720 (scale 1 = 0.3 of it)
    const float s = 0.3f * scale;
    Flat flat { dl, center, s,
                toColor(getInputDisplayColor(set->mInputDisplayButtonColor)),
                toColor(getInputDisplayColor(set->mInputDisplayButtonPressedColor, 0.35f)),
                toColor(getInputDisplayColor(set->mInputDisplayStickColor, 0.55f)),
                toColor(getInputDisplayColor(set->mInputDisplayRingColor)), npad };

    // The model, in the middle top, turning with the motion
    Vec3 axes[3];
    Projector pr;
    bool isJoyCons = m.style == cStyleJoyDual || m.style == cStyleHandheld;
    if (isJoyCons) {
        // Two Joy-Con, each turning with its own sensor; same colors as the Pro Controller
        ModelColors colors { IM_COL32(45, 46, 50, 255), IM_COL32(25, 25, 28, 255), IM_COL32(25, 25, 28, 255),
                             IM_COL32(160, 160, 160, 255), IM_COL32(232, 232, 232, 255), flat.pressed };
        for (int side = 0; side < 2; side++) {
            relativeAxes(m.count > side ? m.sensors[side] : sNoSensor, axes);
            for (int i = 0; i < 3; i++) pr.axes[i] = axes[i];
            // Centered in the free space between the sticks / D-Pad and A B X Y
            pr.center = flat.at(side == 0 ? -144 : 56, -130);
            if (side == 0) {
                drawMesh(dl, pr, 92 * s, cJoyLVerts, cJoyLTris, cJoyLParts, colors, npad);
            } else {
                drawMesh(dl, pr, 92 * s, cJoyRVerts, cJoyRTris, cJoyRParts, colors, npad);
            }
        }
    } else {
        // Pro Controller: dark body, light buttons, gray sticks
        ModelColors colors { IM_COL32(45, 46, 50, 255), IM_COL32(25, 25, 28, 255), IM_COL32(25, 25, 28, 255),
                             IM_COL32(160, 160, 160, 255), IM_COL32(232, 232, 232, 255), flat.pressed };
        relativeAxes(m.count > 0 ? m.sensors[0] : sNoSensor, axes);
        for (int i = 0; i < 3; i++) pr.axes[i] = axes[i];
        pr.center = flat.at(-5, -138);
        drawMesh(dl, pr, 76 * s, cProVerts, cProTris, cProParts, colors, npad);
    }

    // The flat input display around it
    flat.pill(-427, -300, P::cPadIdx_ZL, "ZL");
    flat.pill(-427, -220, P::cPadIdx_L, "L");
    flat.pill(381, -300, P::cPadIdx_ZR, "ZR");
    flat.pill(381, -220, P::cPadIdx_R, "R");
    flat.stickAt(-427, 5, true);
    flat.dpad(-185, 185);
    flat.stickAt(138, 185, false);
    // - and +: on the Pro Controller's own, beside the Joy-Con
    flat.small(isJoyCons ? -262 : -188, -100, P::cPadIdx_Minus, "-");
    flat.small(isJoyCons ? 174 : 135, -100, P::cPadIdx_Plus, "+");
    flat.button(381, -98, P::cPadIdx_X, "X");
    flat.button(279, 3, P::cPadIdx_Y, "Y");
    flat.button(481, 3, P::cPadIdx_A, "A");
    flat.button(381, 105, P::cPadIdx_B, "B");
    return true;
}

} // namespace btt
