#include "menu/QuickWheel.h"
#include "menu/Lang.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "helpers/InputHelper.h"
#include "imgui.h"
#include "menu/Menu.h"
#include "menu/MenuComponent.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

namespace {

    constexpr int cMaxActions = 96;

    struct Action {
        const char* name;
        MenuComponent* component; // on/off option or button of the menu
        void (*extra)(Menu* menu); // or an action that only exists as a hotkey
    };

    Action sActions[cMaxActions];
    const char* sActionNames[cMaxActions];
    int sActionNum = 0;
    volatile bool sIsReady = false;

    bool sIsOpen = false;
    int sSelected = -1;
    // Slot -> action index, refreshed from the saved names
    int sSlotActions[cWheelSlotNum] = {};
    char sSlotNamesSeen[cWheelSlotNum][32] = {};

    void addAction(const char* name, MenuComponent* component, void (*extra)(Menu*)) {
        if (sActionNum >= cMaxActions || !name || !name[0]) return;
        for (int i = 1; i < sActionNum; i++)
            if (strcmp(sActions[i].name, name) == 0) return;
        sActions[sActionNum] = { name, component, extra };
        sActionNames[sActionNum] = name;
        sActionNum++;
    }

    // Saved slot names -> action indices (when they change: settings loaded, slot edited in the menu)
    void refreshSlots() {
        SettingsMgr::Settings* s = SettingsMgr::getSettings();
        if (!s) return;
        for (int i = 0; i < cWheelSlotNum; i++) {
            if (strncmp(sSlotNamesSeen[i], s->mWheelSlots[i], sizeof(sSlotNamesSeen[i])) == 0) continue;
            memcpy(sSlotNamesSeen[i], s->mWheelSlots[i], sizeof(sSlotNamesSeen[i]));
            sSlotActions[i] = findWheelAction(s->mWheelSlots[i]);
        }
    }

    int wheelSize() {
        SettingsMgr::Settings* s = SettingsMgr::getSettings();
        int i = s ? s->mWheelSize : 1;
        return cWheelSizes[i < int(sizeof(cWheelSizes) / sizeof(cWheelSizes[0])) ? i : 1];
    }

    u8 wheelKey() {
        SettingsMgr::Settings* s = SettingsMgr::getSettings();
        return s ? s->mWheelKey : 0;
    }

    bool isHoldMode() { return wheelKey() == 4 || wheelKey() == 5; }

    bool isClickPressed() {
        switch (wheelKey()) {
        case 1: return InputHelper::isPressStickL();
        case 2: return InputHelper::isPressStickR();
        case 3: return InputHelper::isPressStickL() || InputHelper::isPressStickR();
        default: return false;
        }
    }

    bool isHoldKeyHeld() { return wheelKey() == 4 ? InputHelper::isHoldL() : InputHelper::isHoldR(); }
    bool isHoldKeyPressed() { return wheelKey() == 4 ? InputHelper::isPressL() : InputHelper::isPressR(); }

    // A hotkey button pressed while a hold key opens the wheel (L / R + D-Pad, ZL, ZR or the other shoulder): it was
    // a hotkey, cancel. A / B / X / Y and the sticks stay free to play while pointing at a slot.
    bool isOtherButtonPressed() {
        return InputHelper::isPressZL() || InputHelper::isPressZR() || InputHelper::isPressPadUp() ||
               InputHelper::isPressPadDown() || InputHelper::isPressPadLeft() || InputHelper::isPressPadRight() ||
               (wheelKey() == 4 ? InputHelper::isPressR() : InputHelper::isPressL());
    }

    // Slot pointed by the right stick (0 = up, clockwise), -1 at rest
    int pointedSlot() {
        float x = InputHelper::getRightStickX(), y = InputHelper::getRightStickY();
        if (x * x + y * y < 0.5f * 0.5f) return -1;
        float angle = atan2f(x, y); // 0 = up, clockwise
        if (angle < 0) angle += 2 * 3.14159265f;
        int n = wheelSize();
        return int(angle / (2 * 3.14159265f / n) + 0.5f) % n;
    }

    void runSlot(Menu* menu, int slot) {
        if (slot < 0 || slot >= wheelSize() || sSlotActions[slot] == 0) return;
        const Action& a = sActions[sSlotActions[slot]];
        if (a.component) {
            a.component->onAction();
            if (bool* value = a.component->getWheelToggle())
                menu->setPopupText("%s: %s", tr(a.name), tr(*value ? "ON" : "OFF"));
            else
                menu->setPopupText("%s", tr(a.name));
        } else if (a.extra) {
            a.extra(menu);
            menu->setPopupText("%s", tr(a.name));
        }
    }

    void warpLastCheckpoint(Menu* menu) { menu->warpLastCheckpoint(); }

} // namespace

void prepareQuickWheel(Menu* menu) {
    if (!sIsReady && menu) menu->initCategories();
}

void buildWheelActions(Menu* menu) {
    if (sIsReady || !menu) return;
    sActionNum = 0;
    sActions[0] = { "Empty", nullptr, nullptr };
    sActionNames[0] = "Empty";
    sActionNum = 1;
    // Options, Misc, Timer: every on/off option and every button
    for (int cat : { 0, 2, 3 }) {
        for (MenuComponent* c : menu->mCategories[cat].components)
            if (c && c->getWheelName()) addAction(c->getWheelName(), c, nullptr);
    }
    addAction("Warp Last CP", nullptr, warpLastCheckpoint);
    sIsReady = true;
}

int getWheelActionNum() {
    return sIsReady ? sActionNum : 1;
}

const char* const* getWheelActionNames() {
    static const char* const sEmpty[] = { "Empty" };
    return sIsReady ? sActionNames : sEmpty;
}

int findWheelAction(const char* name) {
    if (!sIsReady || !name || !name[0]) return 0;
    for (int i = 1; i < sActionNum; i++)
        if (strcmp(sActions[i].name, name) == 0) return i;
    return 0;
}

bool isQuickWheelOpen() {
    return sIsOpen;
}

bool isQuickWheelBlockingHotkeys() {
    return sIsOpen && !isHoldMode();
}

void updateQuickWheel(Menu* menu) {
    if (!sIsReady || !menu) return;
    refreshSlots();
    if (menu->mIsEnabledMenu || wheelKey() == 0) {
        sIsOpen = false;
        return;
    }
    if (!sIsOpen) {
        if (isHoldMode() ? isHoldKeyPressed() : isClickPressed()) {
            sIsOpen = true;
            sSelected = -1;
        }
        return;
    }
    // The right stick points at a slot; Mario keeps moving and A stays with the game
    int pointed = pointedSlot();
    if (pointed >= 0) sSelected = pointed;
    if (isHoldMode()) {
        // L / R held: releasing it runs the slot pointed at
        if (isOtherButtonPressed()) {
            sIsOpen = false;
        } else if (!isHoldKeyHeld()) {
            sIsOpen = false;
            runSlot(menu, sSelected);
        }
        return;
    }
    // L3 / R3: letting the right stick go back to the middle after pointing at a slot runs it; clicking again closes
    // the wheel without doing anything
    if (isClickPressed()) {
        sIsOpen = false;
    } else if (pointed < 0 && sSelected >= 0) {
        sIsOpen = false;
        runSlot(menu, sSelected);
    }
}

void drawQuickWheel() {
    if (!sIsOpen || !sIsReady) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const ImVec2 c(800.f, 450.f);
    constexpr float cOuter = 280.f, cInner = 70.f, cPi = 3.14159265f;
    const int n = wheelSize();
    const float step = 2 * cPi / n;

    dl->AddCircleFilled(c, cOuter, IM_COL32(15, 15, 25, 200), 64);
    for (int i = 0; i < n; i++) {
        // Slot i is centered on angle i * step from the top, clockwise (ImGui angles: 0 = right, clockwise)
        float a0 = -cPi / 2 + (i - 0.5f) * step, a1 = -cPi / 2 + (i + 0.5f) * step;
        if (i == sSelected) {
            dl->PathArcTo(c, cOuter - 4, a0, a1, 16);
            dl->PathArcTo(c, cInner, a1, a0, 8);
            dl->PathFillConvex(IM_COL32(230, 80, 120, 170));
        }
        ImVec2 edge(c.x + cosf(a0) * cOuter, c.y + sinf(a0) * cOuter);
        ImVec2 inner(c.x + cosf(a0) * cInner, c.y + sinf(a0) * cInner);
        dl->AddLine(inner, edge, IM_COL32(255, 255, 255, 60), 2.f);

        const Action& a = sActions[sSlotActions[i]];
        const char* label = sSlotActions[i] ? tr(a.name) : "-";
        float mid = -cPi / 2 + i * step, r = (cOuter + cInner) / 2 + 10;
        ImVec2 p(c.x + cosf(mid) * r, c.y + sinf(mid) * r);
        float size = n > 8 ? 18.f : 22.f;
        ImVec2 ts = font->CalcTextSizeA(size, 150.f, 150.f, label);
        ImU32 col = IM_COL32(255, 255, 255, sSlotActions[i] ? 255 : 110);
        dl->AddText(font, size, ImVec2(p.x - ts.x / 2, p.y - ts.y / 2 - 8), col, label, nullptr, 150.f);
        if (sSlotActions[i] && a.component) {
            if (bool* value = a.component->getWheelToggle()) {
                const char* state = tr(*value ? "ON" : "OFF");
                ImVec2 ss = font->CalcTextSizeA(18.f, FLT_MAX, 0.f, state);
                dl->AddText(font, 18.f, ImVec2(p.x - ss.x / 2, p.y + ts.y / 2 - 6),
                            *value ? IM_COL32(110, 230, 120, 255) : IM_COL32(240, 110, 110, 255), state);
            }
        }
    }
    dl->AddCircle(c, cOuter, IM_COL32(255, 255, 255, 90), 64, 2.f);
    dl->AddCircleFilled(c, cInner, IM_COL32(30, 30, 45, 230), 48);
    const char* hint = tr(isHoldMode() ? "Release: OK" : "Release the stick: OK");
    ImVec2 hs = font->CalcTextSizeA(16.f, FLT_MAX, 0.f, hint);
    dl->AddText(font, 16.f, ImVec2(c.x - hs.x / 2, c.y - hs.y / 2), IM_COL32(255, 255, 255, 200), hint);
}

} // namespace btt
