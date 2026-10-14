#include "menu/Lang.h"
#include "menu/Keyboard.h"

#include <cstdio>
#include <cstring>

#include "helpers/InputHelper.h"
#include "imgui.h"

namespace btt {

namespace {

    constexpr int cMaxBytes = 96;
    constexpr int cCols = 10;

    // Letter rows (lower case shown unless caps); each key is one UTF-8 character
    const char* const cLower[][cCols] = {
        { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" },
        { "k", "l", "m", "n", "o", "p", "q", "r", "s", "t" },
        { "u", "v", "w", "x", "y", "z", "é", "è", "ê", "à" },
        { "ç", "ù", "ô", "î", "ë", "ï", "â", "û", "ü", "ö" },
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
        { "'", "-", "!", "?", ".", ",", ":", "&", "(", ")" },
    };
    const char* const cUpper[][cCols] = {
        { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J" },
        { "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T" },
        { "U", "V", "W", "X", "Y", "Z", "É", "È", "Ê", "À" },
        { "Ç", "Ù", "Ô", "Î", "Ë", "Ï", "Â", "Û", "Ü", "Ö" },
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
        { "\"", "/", "+", "*", "=", "%", "#", "@", "~", "_" },
    };
    constexpr int cLetterRows = sizeof(cLower) / sizeof(cLower[0]);
    // Last row: wide action keys
    enum Action { Space, Caps, Erase, Validate, Cancel, ActionNum };
    const char* const cActionNames[ActionNum] = { "Space", "Caps", "Erase", "OK (+)", "Cancel (-)" };

    bool sIsOpen = false;
    bool sIsCaps = true;
    int sRow = 0, sCol = 0;
    char sTitle[96] = {};
    char sText[cMaxBytes + 1] = {};
    std::function<void(const char*)> sOnValidate;

    int actionOfCol(int col) { return col * ActionNum / cCols; }
    int colOfAction(int action) { return action * cCols / ActionNum; }

    void append(const char* s) {
        size_t len = strlen(sText), add = strlen(s);
        if (len + add > cMaxBytes) return;
        memcpy(sText + len, s, add + 1);
        // Caps only for the first letter, like a phone keyboard
        if (sIsCaps && len == 0) sIsCaps = false;
    }

    void erase() {
        size_t len = strlen(sText);
        if (!len) return;
        // remove a whole UTF-8 character
        do {
            len--;
        } while (len > 0 && ((unsigned char)sText[len] & 0xC0) == 0x80);
        sText[len] = '\0';
    }

    void close(bool validate) {
        sIsOpen = false;
        if (validate && sOnValidate) sOnValidate(sText);
        sOnValidate = nullptr;
    }

    void press() {
        if (sRow < cLetterRows) {
            append((sIsCaps ? cUpper : cLower)[sRow][sCol]);
            return;
        }
        switch (actionOfCol(sCol)) {
        case Space: append(" "); break;
        case Caps: sIsCaps = !sIsCaps; break;
        case Erase: erase(); break;
        case Validate: close(true); break;
        case Cancel: close(false); break;
        default: break;
        }
    }

} // namespace

void openKeyboard(const char* title, const char* initialText, std::function<void(const char*)> onValidate) {
    snprintf(sTitle, sizeof(sTitle), "%s", title ? tr(title) : "");
    snprintf(sText, sizeof(sText), "%s", initialText ? initialText : "");
    sOnValidate = onValidate;
    sIsCaps = sText[0] == '\0';
    sRow = 0;
    sCol = 0;
    sIsOpen = true;
}

bool isKeyboardOpen() {
    return sIsOpen;
}

void updateKeyboardInput() {
    if (!sIsOpen) return;
    const int rows = cLetterRows + 1;
    if (InputHelper::isPressPadUp()) sRow = (sRow + rows - 1) % rows;
    else if (InputHelper::isPressPadDown()) sRow = (sRow + 1) % rows;
    else if (InputHelper::isPressPadLeft()) {
        if (sRow == cLetterRows) sCol = colOfAction((actionOfCol(sCol) + ActionNum - 1) % ActionNum);
        else sCol = (sCol + cCols - 1) % cCols;
    } else if (InputHelper::isPressPadRight()) {
        if (sRow == cLetterRows) sCol = colOfAction((actionOfCol(sCol) + 1) % ActionNum);
        else sCol = (sCol + 1) % cCols;
    } else if (InputHelper::isPressA()) press();
    else if (InputHelper::isPressB()) erase();
    else if (InputHelper::isPressY()) sIsCaps = !sIsCaps;
    else if (InputHelper::isPressPlus()) close(true);
    else if (InputHelper::isPressMinus()) close(false);
}

void drawKeyboard() {
    if (!sIsOpen) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    if (!font) return;

    constexpr float keyW = 74, keyH = 46, gap = 6;
    const float panelW = cCols * (keyW + gap) + gap + 20;
    const float panelH = 150 + (cLetterRows + 1) * (keyH + gap) + 40;
    const ImVec2 tl(800 - panelW / 2, 450 - panelH / 2);
    dl->AddRectFilled(tl, ImVec2(tl.x + panelW, tl.y + panelH), IM_COL32(10, 10, 16, 235), 18);
    dl->AddRect(tl, ImVec2(tl.x + panelW, tl.y + panelH), IM_COL32(255, 255, 255, 60), 18.0f, ImDrawFlags_None, 2.0f);

    dl->AddText(font, 32, ImVec2(tl.x + 20, tl.y + 14), IM_COL32(255, 215, 0, 255), sTitle);
    // Text field with a blinking cursor
    ImVec2 fieldTl(tl.x + 20, tl.y + 60), fieldBr(tl.x + panelW - 20, tl.y + 116);
    dl->AddRectFilled(fieldTl, fieldBr, IM_COL32(255, 255, 255, 30), 10);
    char shown[cMaxBytes + 2];
    bool blink = (ImGui::GetFrameCount() / 30) % 2 == 0;
    snprintf(shown, sizeof(shown), "%s%s", sText, blink ? "|" : "");
    dl->AddText(font, 36, ImVec2(fieldTl.x + 12, fieldTl.y + 8), IM_COL32(255, 255, 255, 255), shown);

    float y = tl.y + 132;
    for (int r = 0; r < cLetterRows; r++) {
        for (int c = 0; c < cCols; c++) {
            ImVec2 k(tl.x + 10 + gap + c * (keyW + gap), y);
            bool sel = sRow == r && sCol == c;
            dl->AddRectFilled(k, ImVec2(k.x + keyW, k.y + keyH), sel ? IM_COL32(255, 60, 60, 200) : IM_COL32(255, 255, 255, 40), 8);
            const char* label = (sIsCaps ? cUpper : cLower)[r][c];
            ImVec2 ts = font->CalcTextSizeA(30, FLT_MAX, 0, label);
            dl->AddText(font, 30, ImVec2(k.x + (keyW - ts.x) / 2, k.y + (keyH - ts.y) / 2), IM_COL32(255, 255, 255, 255), label);
        }
        y += keyH + gap;
    }
    const float actW = (cCols * (keyW + gap) - gap - (ActionNum - 1) * gap) / ActionNum;
    for (int a = 0; a < ActionNum; a++) {
        ImVec2 k(tl.x + 10 + gap + a * (actW + gap), y);
        bool sel = sRow == cLetterRows && actionOfCol(sCol) == a;
        ImU32 base = a == Validate ? IM_COL32(60, 170, 80, 120) : a == Cancel ? IM_COL32(170, 60, 60, 120) : IM_COL32(255, 255, 255, 40);
        dl->AddRectFilled(k, ImVec2(k.x + actW, k.y + keyH), sel ? IM_COL32(255, 60, 60, 200) : base, 8);
        const char* label = tr(a == Caps ? (sIsCaps ? "Caps: ON" : "Caps: off") : cActionNames[a]);
        ImVec2 ts = font->CalcTextSizeA(26, FLT_MAX, 0, label);
        dl->AddText(font, 26, ImVec2(k.x + (actW - ts.x) / 2, k.y + (keyH - ts.y) / 2), IM_COL32(255, 255, 255, 255), label);
    }
    y += keyH + gap;
    dl->AddText(font, 22, ImVec2(tl.x + 20, y + 4), IM_COL32(200, 200, 200, 255),
                tr("D-Pad: move   A: type   B: erase   Y: caps   +: OK   -: cancel"));
}

} // namespace btt
