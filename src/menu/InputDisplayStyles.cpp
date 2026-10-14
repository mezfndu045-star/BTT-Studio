#include "menu/InputDisplayStyles.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"

#include "sead/controller/seadControllerMgr.h"

#include "imgui_internal.h"
#include "menu/InputDisplay.h"
#include "menu/InputDisplaySkins.h"
#include "menu/Menu.h"

namespace btt {

void drawStyledInputDisplay(int port, ImVec2* settingPos, const char* dragId, const char* label) {
    SettingsMgr::Settings* set = SettingsMgr::getSettings();
    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();
    auto* npad = (al::NpadController*)controllerMgr->getController(al::getPlayerControllerPort(port));
    if (!npad) return;

    // Same area as the classic display (it is drawn above-left of the saved position)
    const ImVec2 center(settingPos->x - 125, settingPos->y - 95);
    constexpr float cMaxW = 330, cMaxH = 230;
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    if (Menu::instance() && Menu::instance()->mIsEnabledMenu) {
        ImVec2 dragMin(center.x - cMaxW / 2 - 10, center.y - cMaxH / 2 - 10);
        ImVec2 dragMax(center.x + cMaxW / 2 + 10, center.y + cMaxH / 2 + 10);
        dl->AddRect(dragMin, dragMax, IM_COL32(255, 255, 255, 45), 10.0f, ImDrawFlags_None, 1.5f);
        ImGui::SetCursorScreenPos(dragMin);
        ImGui::InvisibleButton(dragId, { dragMax.x - dragMin.x, dragMax.y - dragMin.y });
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            settingPos->x += delta.x;
            settingPos->y += delta.y;
        }
    }

    int skin = set->mInputDisplayStyle - u8(InputDisplayStyle::Periscope);
    if (skin < 0 || skin >= int(sizeof(sInputDisplaySkinIds) / sizeof(sInputDisplaySkinIds[0]))) {
        // A style that no longer exists (older settings): back to Classic
        set->mInputDisplayStyle = u8(InputDisplayStyle::Classic);
        return;
    }
    ImVec2 skinMin, skinMax;
    if (!drawSkinInputDisplay(sInputDisplaySkinIds[skin], npad, center, cMaxW, cMaxH, &skinMin, &skinMax)) return;

    if (label) {
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font->CalcTextSizeA(24, FLT_MAX, 0.f, label);
        dl->AddText(font, 24, ImVec2(center.x - ts.x / 2, skinMin.y - ts.y - 4), IM_COL32(255, 255, 255, 255), label);
    }
}

} // namespace btt
