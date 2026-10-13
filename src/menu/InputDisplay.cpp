#include "menu/InputDisplay.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"

#include "sead/controller/seadControllerMgr.h"

#include "imgui_internal.h"
#include "menu/Menu.h"

namespace btt {

using InputCallback = bool;

static ImU32 makeColor(const ImVec4& color) {
    return IM_COL32(u8(color.x), u8(color.y), u8(color.z), u8(color.w));
}

static void drawButton(
    const ImVec2& pos, InputCallback callback, float radius = 8,
    float phaseOffset = 0.0f
) {
    SettingsMgr* set = SettingsMgr::instance();
    ImU32 col = callback
        ? makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayButtonPressedColor, phaseOffset + 0.35f))
        : makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayButtonColor, phaseOffset));
    ImGui::GetForegroundDrawList()->AddCircleFilled(pos, radius, col, 16);
}

static void drawStickButton(
    const ImVec2& pos, InputCallback callback, float radius = 16,
    float phaseOffset = 0.0f
) {
    SettingsMgr* set = SettingsMgr::instance();
    ImU32 col = callback
        ? makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayButtonPressedColor, phaseOffset + 0.35f))
        : makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayStickColor, phaseOffset));
    ImGui::GetForegroundDrawList()->AddCircleFilled(pos, radius, col, 16);
}

static void drawButtonRect(
    const ImVec2& pos, InputCallback callback,
    float phaseOffset = 0.0f
) {
    SettingsMgr* set = SettingsMgr::instance();
    ImU32 col = callback
        ? makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayButtonPressedColor, phaseOffset + 0.35f))
        : makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayButtonColor, phaseOffset));
    const ImVec2 min(pos.x - 13, pos.y - 6);
    const ImVec2 max(pos.x + 13, pos.y + 6);
    ImGui::GetForegroundDrawList()->AddRectFilled(min, max, col, 10.0f);
}

void drawInputDisplay() {
    SettingsMgr* set = SettingsMgr::instance();
    if (!set->getSettings()->mIsEnableInputDisplay) return;

    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();
    al::NpadController* controller =
        (al::NpadController*)controllerMgr->getController(al::getPlayerControllerPort(0));

    const sead::Vector2f leftStick = { controller->mLeftStick.x, controller->mLeftStick.y };
    const sead::Vector2f rightStick = { controller->mRightStick.x, controller->mRightStick.y };

    ImVec2 pos = set->getSettings()->mInputDisplayPos;
    pos.x -= 200;
    pos.y -= 100;

    // Draggable zone with mouse when menu is open
    if (btt::Menu::instance()->mIsEnabledMenu) {
        ImVec2 dragMin = { pos.x - 55, pos.y - 185 };
        ImVec2 dragSize = { 260, 290 };
        ImGui::GetForegroundDrawList()->AddRect(
            dragMin, { dragMin.x + dragSize.x, dragMin.y + dragSize.y },
            IM_COL32(255, 255, 255, 45), 10.0f, 0, 1.5f
        );
        ImGui::SetCursorScreenPos(dragMin);
        ImGui::InvisibleButton("##InputDisplayDragP1", dragSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            set->getSettings()->mInputDisplayPos.x += delta.x;
            set->getSettings()->mInputDisplayPos.y += delta.y;
        }
    }

    if (set->getSettings()->mIsEnableInput2P) {
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetDefaultFont(), 27.f, { pos.x + 20, pos.y - 100 },
            makeColor(getInputDisplayColor(SettingsMgr::InputDisplayColor::White)), "Player 1"
        );
    }

    if (set->getSettings()->mInputDisplayBackColor != SettingsMgr::InputDisplayColor::None) {
        ImVec4 color = getInputDisplayColor(set->getSettings()->mInputDisplayBackColor, 0.0f);
        color.w = 140;
        ImGui::GetForegroundDrawList()->AddRectFilled(
            { pos.x - 50, pos.y - 100 }, { pos.x + 200, pos.y + 100 }, makeColor(color), 20.0f
        );
    }

    ImGui::GetForegroundDrawList()->AddCircle(
        pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor, 0.0f)), 0, 2
    );
    ImVec2 leftPos = { pos.x + leftStick.x * 30, pos.y - leftStick.y * 30 };
    drawStickButton(
        leftPos, controller->mPadHold.isOnBit(controller->cPadIdx_1) /*StickL*/, 16, 0.05f
    );

    pos.x += 40;
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Up), 8, 0.1f); // up
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Down), 8, 0.2f); // down
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Left), 8, 0.3f); // left
    pos.x += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Right), 8, 0.4f); // right

    pos.x += 60;
    ImGui::GetForegroundDrawList()->AddCircle(
        pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor, 0.5f)), 0, 2
    );
    ImVec2 rightPos = { pos.x + rightStick.x * 30, pos.y - rightStick.y * 30 };
    drawStickButton(
        rightPos, controller->mPadHold.isOnBit(controller->cPadIdx_2) /*StickR*/, 16, 0.55f
    );

    pos.x += 40;
    pos.y -= 60;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_X), 8, 0.6f); // x
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_B), 8, 0.7f); // b
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Y), 8, 0.8f); // y
    pos.x += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_A), 8, 0.9f); // a

    pos.y -= 10;
    pos.x -= 75;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Plus), 5, 0.95f); // plus
    pos.x -= 40;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Minus), 5, 0.05f); // minus

    pos = set->getSettings()->mInputDisplayPos;
    pos.x -= 200;
    pos.y -= 160;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_L), 0.15f); // l
    pos.y -= 16;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_ZL), 0.25f); // zl
    pos.x += 155;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_ZR), 0.75f); // zr
    pos.y += 16;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_R), 0.85f); // r
}

void drawInputDisplayP2() {
    SettingsMgr* set = SettingsMgr::instance();
    if (!set->getSettings()->mIsEnableInputDisplay || !set->getSettings()->mIsEnableInput2P) return;

    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();
    al::NpadController* controller =
        (al::NpadController*)controllerMgr->getController(al::getPlayerControllerPort(1));

    const sead::Vector2f leftStick = { controller->mLeftStick.x, controller->mLeftStick.y };
    const sead::Vector2f rightStick = { controller->mRightStick.x, controller->mRightStick.y };

    ImVec2 pos = set->getSettings()->mInputDisplayPosP2;
    pos.x -= 200;
    pos.y -= 100;

    // Draggable zone with mouse when menu is open
    if (btt::Menu::instance()->mIsEnabledMenu) {
        ImVec2 dragMin = { pos.x - 55, pos.y - 185 };
        ImVec2 dragSize = { 260, 290 };
        ImGui::GetForegroundDrawList()->AddRect(
            dragMin, { dragMin.x + dragSize.x, dragMin.y + dragSize.y },
            IM_COL32(255, 255, 255, 45), 10.0f, 0, 1.5f
        );
        ImGui::SetCursorScreenPos(dragMin);
        ImGui::InvisibleButton("##InputDisplayDragP2", dragSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            set->getSettings()->mInputDisplayPosP2.x += delta.x;
            set->getSettings()->mInputDisplayPosP2.y += delta.y;
        }
    }

    ImGui::GetForegroundDrawList()->AddText(
        ImGui::GetDefaultFont(), 27.f, { pos.x + 20, pos.y - 100 },
        makeColor(getInputDisplayColor(SettingsMgr::InputDisplayColor::White)), "Player 2"
    );

    if (set->getSettings()->mInputDisplayBackColor != SettingsMgr::InputDisplayColor::None) {
        ImVec4 color = getInputDisplayColor(set->getSettings()->mInputDisplayBackColor, 0.0f);
        color.w = 140;
        ImGui::GetForegroundDrawList()->AddRectFilled(
            { pos.x - 50, pos.y - 100 }, { pos.x + 200, pos.y + 100 }, makeColor(color), 20.0f
        );
    }

    ImGui::GetForegroundDrawList()->AddCircle(
        pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor, 0.0f)), 0, 2
    );
    ImVec2 leftPos = { pos.x + leftStick.x * 30, pos.y - leftStick.y * 30 };
    drawStickButton(
        leftPos, controller->mPadHold.isOnBit(controller->cPadIdx_1) /*StickL*/, 16, 0.05f
    );

    pos.x += 40;
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Up), 8, 0.1f); // up
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Down), 8, 0.2f); // down
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Left), 8, 0.3f); // left
    pos.x += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Right), 8, 0.4f); // right

    pos.x += 60;
    ImGui::GetForegroundDrawList()->AddCircle(
        pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor, 0.5f)), 0, 2
    );
    ImVec2 rightPos = { pos.x + rightStick.x * 30, pos.y - rightStick.y * 30 };
    drawStickButton(
        rightPos, controller->mPadHold.isOnBit(controller->cPadIdx_2) /*StickR*/, 16, 0.55f
    );

    pos.x += 40;
    pos.y -= 60;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_X), 8, 0.6f); // x
    pos.y += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_B), 8, 0.7f); // b
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Y), 8, 0.8f); // y
    pos.x += 30;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_A), 8, 0.9f); // a

    pos.y -= 10;
    pos.x -= 75;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Plus), 5, 0.95f); // plus
    pos.x -= 40;
    drawButton(pos, controller->mPadHold.isOnBit(controller->cPadIdx_Minus), 5, 0.05f); // minus

    pos = set->getSettings()->mInputDisplayPosP2;
    pos.x -= 200;
    pos.y -= 160;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_L), 0.15f); // l
    pos.y -= 16;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_ZL), 0.25f); // zl
    pos.x += 155;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_ZR), 0.75f); // zr
    pos.y += 16;
    drawButtonRect(pos, controller->mPadHold.isOnBit(controller->cPadIdx_R), 0.85f); // r
}

} // namespace btt
