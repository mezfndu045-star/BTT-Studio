#include "imgui.h"
#include "menu/InputDisplay.h"
#include "menu/Menu.h"

using namespace btt;

void Menu::drawPageInputDisplay() {
    if (ImGui::CollapsingHeader("Input Display")) {
        int bgColor = (u8)set->getSettings()->mInputDisplayBackColor;
        int ringColor = (u8)set->getSettings()->mInputDisplayRingColor;
        int stickColor = (u8)set->getSettings()->mInputDisplayStickColor;
        int buttonPressedColor = (u8)set->getSettings()->mInputDisplayButtonPressedColor;
        int buttonColor = (u8)set->getSettings()->mInputDisplayButtonColor;
        int posX = (int)set->getSettings()->mInputDisplayPos.x;
        int posY = (int)set->getSettings()->mInputDisplayPos.y;
        int posXP2 = (int)set->getSettings()->mInputDisplayPosP2.x;
        int posYP2 = (int)set->getSettings()->mInputDisplayPosP2.y;

        ImGui::Indent();
        ImGui::Checkbox("Toggle", &set->getSettings()->mIsEnableInputDisplay);
        ImGui::Checkbox("2P Mode", &set->getSettings()->mIsEnableInput2P);

        ImGui::PushItemWidth(200);

        if (ImGui::Combo(
                "Button Color", &buttonColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)
            ))
            set->getSettings()->mInputDisplayButtonColor = (SettingsMgr::InputDisplayColor)buttonColor;

        if (ImGui::Combo(
                "Pressed Color", &buttonPressedColor, sInputDisplayColorNames,
                IM_ARRAYSIZE(sInputDisplayColorNames)
            ))
            set->getSettings()->mInputDisplayButtonPressedColor =
                (SettingsMgr::InputDisplayColor)buttonPressedColor;

        if (ImGui::Combo(
                "Stick Color", &stickColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)
            ))
            set->getSettings()->mInputDisplayStickColor = (SettingsMgr::InputDisplayColor)stickColor;

        if (ImGui::Combo(
                "Ring Color", &ringColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)
            ))
            set->getSettings()->mInputDisplayRingColor = (SettingsMgr::InputDisplayColor)ringColor;

        if (ImGui::Combo(
                "BG Color", &bgColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)
            ))
            set->getSettings()->mInputDisplayBackColor = (SettingsMgr::InputDisplayColor)bgColor;

        if (ImGui::InputInt("Position X", &posX, 25)) set->getSettings()->mInputDisplayPos.x = (float)posX;
        if (ImGui::InputInt("Position Y", &posY, 25)) set->getSettings()->mInputDisplayPos.y = (float)posY;
        ImGui::PopItemWidth();

        if (ImGui::Button("Bottom Left")) set->getSettings()->mInputDisplayPos = ImVec2(250.f, 720.f);
        ImGui::SameLine();
        if (ImGui::Button("Bottom Right")) set->getSettings()->mInputDisplayPos = ImVec2(1380.f, 720.f);
        ImGui::SameLine();
        if (ImGui::Button("Top Left")) set->getSettings()->mInputDisplayPos = ImVec2(250.f, 220.f);
        ImGui::SameLine();
        if (ImGui::Button("Top Right")) set->getSettings()->mInputDisplayPos = ImVec2(1380.f, 220.f);
        ImGui::SameLine();
        if (ImGui::Button("Center")) set->getSettings()->mInputDisplayPos = ImVec2(800.f, 450.f);

        ImGui::SeparatorText("Player 2");
        ImGui::PushItemWidth(200);

        if (ImGui::InputInt("Position X P2", &posXP2, 25)) set->getSettings()->mInputDisplayPosP2.x = (float)posXP2;
        if (ImGui::InputInt("Position Y P2", &posYP2, 25)) set->getSettings()->mInputDisplayPosP2.y = (float)posYP2;
        ImGui::PopItemWidth();
        if (ImGui::Button("Reset Position P2")) {
            set->getSettings()->mInputDisplayPosP2 = ImVec2(800.f, 450.f);
        }

        ImGui::Unindent();
    }
}
