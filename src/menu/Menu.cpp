#include "menu/CustomKeys.h"
#include "menu/Menu.h"

#include <cmath>

#include "nn/oe.h"

#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"

#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/System/GameDataFunction.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "helpers/getHelper.h"
#include "helpers/InputHelper.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "menu/stage_warp.h"
#include "menu/InputDisplayStyles.h"
#include "menu/QuickWheel.h"
#include "menu/DebugCommands.h"
#include "menu/PhotoTourList.h"
#include "menu/Keyboard.h"
#include "menu/MoonList.h"
#include "menu/MoonNames.h"
#include "menu/PhotoTour.h"
#include "menu/SubAreaImages.h"
#include "menu/SubAreaList.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

// Misc > Moon: Left/Right picks a moon of the kingdom chosen above, A opens the keyboard to rename it
class MoonRenameMenuComponent : public MenuComponent {
    const int* mKingdom;
    int mIndex = 0;
    int mLastKingdom = -1;
    int mFontSize = 34;

    const KingdomMoons& kingdom() const { return sKingdomMoons[*mKingdom]; }

    const char* shownName(bool* isCustom) const {
        const KingdomMoons& k = kingdom();
        if (k.count <= 0) return "(no moon)";
        const MoonEntry& m = k.moons[mIndex < k.count ? mIndex : 0];
        const char* custom = getCustomMoonName(m.stage, m.objId);
        if (isCustom) *isCustom = custom != nullptr;
        return custom ? custom : m.name;
    }

    void clampIndex() {
        if (*mKingdom != mLastKingdom) {
            mLastKingdom = *mKingdom;
            mIndex = 0;
        }
        if (mIndex >= kingdom().count) mIndex = 0;
    }

public:
    explicit MoonRenameMenuComponent(const int* kingdomIndex) : mKingdom(kingdomIndex) {}

    bool canHaveFocus() const override { return true; }

    ImVec2 getSize() const override {
        char buf[200];
        snprintf(buf, sizeof(buf), "Moon %d/%d: %s", mIndex + 1, kingdom().count, shownName(nullptr));
        ImFont* font = ImGui::GetFont();
        ImVec2 ts = font ? font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, buf) : ImVec2(300, (float)mFontSize);
        return ImVec2(ts.x + 24.0f, ts.y + 12.0f);
    }

    void draw(const ImVec2& at) override {
        clampIndex();
        bool isCustom = false;
        const char* name = shownName(&isCustom);
        char title[48];
        snprintf(title, sizeof(title), "Moon %d/%d: ", mIndex + 1, kingdom().count);
        ImFont* font = ImGui::GetFont();
        if (!font) return;
        ImVec2 ts = font->CalcTextSizeA(mFontSize, FLT_MAX, 0.f, title);
        ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12, at.y + 6), IM_COL32(255, 255, 255, 255), title);
        ImU32 col = mIsFocused ? IM_COL32(255, 215, 0, 255) : isCustom ? IM_COL32(120, 255, 120, 255) : IM_COL32(200, 230, 255, 255);
        ImGui::GetForegroundDrawList()->AddText(font, (float)mFontSize, ImVec2(at.x + 12 + ts.x, at.y + 6), col, name);
    }

    void onLeft() override {
        clampIndex();
        int n = kingdom().count;
        if (n > 0) mIndex = (mIndex + n - 1) % n;
    }
    void onRight() override {
        clampIndex();
        int n = kingdom().count;
        if (n > 0) mIndex = (mIndex + 1) % n;
    }

    // A after choosing: type the new name
    void onConfirm() override {
        clampIndex();
        if (kingdom().count <= 0) return;
        const MoonEntry& m = kingdom().moons[mIndex];
        const char* stage = m.stage;
        const char* objId = m.objId;
        openKeyboard("Moon name", shownName(nullptr), [stage, objId](const char* name) {
            setMoonName(stage, objId, name);
            if (Menu::instance()) Menu::instance()->setPopupText("Moon renamed (reload the scene)");
        });
    }

    void resetSelected() {
        clampIndex();
        if (kingdom().count <= 0) return;
        setMoonName(kingdom().moons[mIndex].stage, kingdom().moons[mIndex].objId, "");
    }
};

// Picture of a stage (romfs ImGuiData/SubAreas/<stage>.astc) with its name under it, drawn right of the menu
static void drawStagePreview(const ImVec2& at, const char* stage, const char* name) {
    constexpr float cWidth = 384.0f, cHeight = 216.0f, cPad = 6.0f;
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 tl(at.x, std::min(at.y, 900.0f - cHeight - 2 * cPad - 40.0f));
    ImVec2 br(tl.x + cWidth + 2 * cPad, tl.y + cHeight + 2 * cPad + 40.0f);
    drawList->AddRectFilled(tl, br, IM_COL32(0, 0, 0, 190), 12.0f);
    ImVec2 imgTl(tl.x + cPad, tl.y + cPad);
    ImTextureID image = getSubAreaImage(stage);
    if (image)
        drawList->AddImage(image, imgTl, ImVec2(imgTl.x + cWidth, imgTl.y + cHeight));
    else
        drawList->AddText(ImGui::GetFont(), 28.0f, ImVec2(imgTl.x + 12.0f, imgTl.y + cHeight / 2 - 14.0f),
                          IM_COL32(200, 200, 200, 255), tr("No picture"));
    drawList->AddText(ImGui::GetFont(), 28.0f, ImVec2(imgTl.x + 4.0f, imgTl.y + cHeight + 6.0f), IM_COL32(255, 255, 255, 255),
                      tr(name));
}

// Panel next to the menu: a title and wrapped lines
void drawMenuHelpPanel(const ImVec2& at, const char* title, const char* body) {
    constexpr float cWidth = 460.f, cPad = 12.f, cWrap = cWidth - 2 * cPad;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    if (!title) title = "";
    ImVec2 titleSize = font->CalcTextSizeA(30.f, FLT_MAX, 0.f, title);
    ImVec2 bodySize = font->CalcTextSizeA(24.f, FLT_MAX, cWrap, body);
    float height = cPad + titleSize.y + 6.f + bodySize.y + cPad;
    ImVec2 tl(std::min(at.x, 1600.f - cWidth - 10.f), std::min(at.y, 900.f - height - 10.f));
    dl->AddRectFilled(tl, ImVec2(tl.x + cWidth, tl.y + height), IM_COL32(0, 0, 0, 200), 12.f);
    float y = tl.y + cPad;
    dl->AddText(font, 30.f, ImVec2(tl.x + cPad, y), IM_COL32(255, 255, 255, 255), title);
    y += titleSize.y + 6.f;
    dl->AddText(font, 24.f, ImVec2(tl.x + cPad, y), IM_COL32(215, 215, 225, 255), body, nullptr, cWrap);
}

// Timer: help panel shown next to Start Type / End Type (what the event is) and Start Count / End Count
namespace {

struct TimerTypeHelp {
    TimerHookType type;
    const char* name;
    const char* when; // completes "Starts / Stops when..."
    const char* whenFr; // completes "Démarre / S'arrête quand..."
};

constexpr TimerTypeHelp cTimerTypeHelp[] = {
    { TimerHookType::None, "None", "never by itself: use Start / Stop Timer or the timer hotkeys.",
      "jamais tout seul : utilise « Lancer le chrono » / « Arrêter le chrono » ou les raccourcis du chrono." },
    { TimerHookType::GameStart, "Press Start",
      "A is pressed on the mode select screen of a new game (Start / Start in Assist Mode).",
      "A est appuyé sur l'écran de choix du mode d'une nouvelle partie (Commencer / Commencer en mode assisté)." },
    { TimerHookType::OdysseyPowerUp, "Odyssey Power Up",
      "the screen turns black after \"The Odyssey has powered up!\" (only that black screen counts).",
      "l'écran devient noir après « L'Odyssée a été rechargée ! » (seul cet écran noir compte)." },
    { TimerHookType::BlackScreen, "Black Screen",
      "a black screen fully covers the game: pipes, doors, loading zones, death, Restart Scene...",
      "un écran noir recouvre tout le jeu : tuyaux, portes, zones de chargement, mort, « Recommencer la scène »..." },
    { TimerHookType::WhiteScreen, "White Screen",
      "a white screen fully covers the game: warp to a checkpoint flag or to the Odyssey...",
      "un écran blanc recouvre tout le jeu : téléportation vers un drapeau ou vers l'Odyssée..." },
    { TimerHookType::Capture, "Capture", "Mario captures something (start of the capture animation).",
      "Mario capture quelque chose (début de l'animation de capture)." },
    { TimerHookType::Damage, "Damage", "Mario takes a hit (also counted with No Damage on).",
      "Mario prend un coup (compte aussi avec « Pas de dégâts »)." },
    { TimerHookType::EscapeWire, "Escape Wire",
      "captured Bowser leaves along the last electric wire of the Moon Kingdom escape.",
      "Bowser capturé part le long du dernier fil électrique de la fuite, au Pays de la Lune." },
    { TimerHookType::MoonGet, "Moon Get", "Mario touches a moon (the moment it is grabbed).",
      "Mario touche une lune (au moment où il l'attrape)." },
    { TimerHookType::ShineTick, "Shine Tick", "the moon counter goes up on the moon get screen.",
      "le compteur de lunes augmente sur l'écran où Mario obtient la lune." },
    { TimerHookType::TwoMoons, "2 Moons (Grab)", "Mario touches his 2nd moon (every 2 moons grabbed).",
      "Mario touche sa 2e lune (une fois toutes les 2 lunes attrapées)." },
    { TimerHookType::TwoMoonsTick, "2 Moons (Tick)", "the moon counter goes up for the 2nd time (every 2 moons).",
      "le compteur de lunes augmente pour la 2e fois (une fois toutes les 2 lunes)." },
    { TimerHookType::ShineGrab, "Shine Grab", "the moon get cutscene starts (Mario holds the moon up).",
      "la cinématique de la lune commence (Mario brandit la lune)." },
};

const TimerTypeHelp& findTimerTypeHelp(TimerHookType type) {
    for (const TimerTypeHelp& h : cTimerTypeHelp)
        if (h.type == type) return h;
    return cTimerTypeHelp[0];
}

} // namespace

// Start Type / End Type, with the help panel of the selected type
class TimerTypeMenuComponent : public EnumMenuComponent<int> {
    const int* mIndex;
    const TimerHookType* mTypes;
    bool mIsStart;

public:
    TimerTypeMenuComponent(int* index, const TimerHookType* types, const char* const* names, int count, bool isStart,
                           std::function<void(int)> onChange)
        : EnumMenuComponent<int>(index, names, count, isStart ? "Start Type" : "End Type", 34, onChange),
          mIndex(index), mTypes(types), mIsStart(isStart) {}

    void drawPreview(const ImVec2& at) override {
        const TimerTypeHelp& h = findTimerTypeHelp(mTypes[*mIndex]);
        auto* s = SettingsMgr::getSettings();
        const bool fr = isFrench();
        char body[512];
        int n;
        if (h.type == TimerHookType::None)
            n = fr ? snprintf(body, sizeof(body), "Le chrono %s %s", mIsStart ? "ne démarre" : "ne s'arrête", h.whenFr)
                   : snprintf(body, sizeof(body), "The timer %s %s", mIsStart ? "starts" : "stops", h.when);
        else
            n = fr ? snprintf(body, sizeof(body), "%s quand %s", mIsStart ? "Démarre (à partir de zéro)" : "S'arrête",
                              h.whenFr)
                   : snprintf(body, sizeof(body), "%s when %s", mIsStart ? "Starts (from zero)" : "Stops", h.when);
        if (s && h.type != TimerHookType::None && s->mTimerStartType == s->mTimerEndType && n > 0)
            snprintf(body + n, sizeof(body) - n, "%s",
                     fr ? "\nMême type au départ et à l'arrivée : le 1er événement lance le chrono, le suivant l'arrête "
                          "(ou affiche un temps intermédiaire si « Temps intermédiaire » est activé)."
                        : "\nSame Start and End Type: the 1st event starts the timer, the next one stops it (or shows a "
                          "lap with Timer Split).");
        drawMenuHelpPanel(at, tr(h.name), body);
    }
};

// Start Count / End Count, with an explanation using the current type and count
class TimerCountMenuComponent : public IntMenuComponent<int> {
    const int* mValue;
    bool mIsStart;

public:
    TimerCountMenuComponent(int* value, bool isStart, std::function<void(int)> onChange)
        : IntMenuComponent<int>(value, isStart ? "Start Count" : "End Count", 1, 99, 1, 34, onChange), mValue(value),
          mIsStart(isStart) {}

    void drawPreview(const ImVec2& at) override {
        auto* s = SettingsMgr::getSettings();
        if (!s) return;
        const TimerTypeHelp& h = findTimerTypeHelp(mIsStart ? s->mTimerStartType : s->mTimerEndType);
        char body[512];
        int count = *mValue;
        if (isFrench()) {
            const char* name = tr(h.name);
            const char* verb = mIsStart ? "lancer" : "arrêter";
            const char* does = mIsStart ? "démarre" : "s'arrête";
            if (h.type == TimerHookType::None)
                snprintf(body, sizeof(body), "Nombre d'événements nécessaires pour %s le chrono. Choisis d'abord un type "
                         "%s.", verb, mIsStart ? "de départ" : "d'arrivée");
            else if (count <= 1)
                snprintf(body, sizeof(body),
                         "Nombre de « %s » nécessaires pour %s le chrono.\n1 = le chrono %s au 1er.\nExemple : avec 3, "
                         "il %s au 3e « %s ».",
                         name, verb, does, does, name);
            else
                snprintf(body, sizeof(body),
                         "Nombre de « %s » nécessaires pour %s le chrono.\nEn ce moment : le chrono %s au %de « %s » (les %d "
                         "premiers sont ignorés). Le compte repart de zéro après chaque %s.",
                         name, verb, does, count, name, count - 1, mIsStart ? "départ" : "arrêt");
        } else {
            auto ord = [](int n) { return (n % 100 >= 11 && n % 100 <= 13) ? "th" : n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th"; };
            if (h.type == TimerHookType::None) {
                snprintf(body, sizeof(body), "How many %s events are needed to %s the timer. Choose a %s first.",
                         mIsStart ? "Start Type" : "End Type", mIsStart ? "start" : "stop", mIsStart ? "Start Type" : "End Type");
            } else if (count <= 1) {
                snprintf(body, sizeof(body),
                         "How many \"%s\" are needed to %s the timer.\n1 = the timer %s at the 1st one.\nExample: %s Count 3 = "
                         "it %s at the 3rd \"%s\".",
                         h.name, mIsStart ? "start" : "stop", mIsStart ? "starts" : "stops", mIsStart ? "Start" : "End",
                         mIsStart ? "starts" : "stops", h.name);
            } else {
                snprintf(body, sizeof(body),
                         "How many \"%s\" are needed to %s the timer.\nNow: the timer %s at the %d%s \"%s\" (the first %d "
                         "are skipped). The count starts again after each %s.",
                         h.name, mIsStart ? "start" : "stop", mIsStart ? "starts" : "stops", count, ord(count), h.name,
                         count - 1, mIsStart ? "start" : "stop");
            }
        }
        drawMenuHelpPanel(at, tr(mIsStart ? "Start Count" : "End Count"), body);
    }
};

// Stages > Kingdom: the selected kingdom's picture is shown next to the menu
class KingdomMenuComponent : public EnumMenuComponent<int> {
    const char* const* mNames;
    const char* const* mStages;
    const int* mIndex;

public:
    KingdomMenuComponent(int* index, const char* const* names, const char* const* stages, int count)
        : EnumMenuComponent<int>(index, names, count, "Kingdom"), mNames(names), mStages(stages), mIndex(index) {}

    void drawPreview(const ImVec2& at) override { drawStagePreview(at, mStages[*mIndex], mNames[*mIndex]); }
};

// One line per kingdom in Stages: Left/Right picks a sub area (its picture is shown next to the menu), A warps there
class SubAreaMenuComponent : public EnumMenuComponent<int> {
    const KingdomSubAreas& mKingdom;
    const int* mScenario;
    int mIndex = 0;

public:
    SubAreaMenuComponent(const KingdomSubAreas& kingdom, const int* scenario)
        : EnumMenuComponent<int>(&mIndex, kingdom.names, kingdom.count, kingdom.title), mKingdom(kingdom),
          mScenario(scenario) {}

    // Arrive through the sub area's own entrance (the one of its pipe / door, from the photo tour list): the game then
    // plays the entrance animation (Mario walking in), also after a reload. "start" when there is none.
    void onConfirm() override {
        const char* stage = mKingdom.areas[mIndex].stage;
        const char* label = "start";
        char shot[96];
        snprintf(shot, sizeof(shot), "%s_in", stage);
        for (const PhotoTourShot& s : sPhotoTourShots) {
            if (strcmp(s.shot, shot) == 0 && s.label && s.label[0]) {
                label = s.label;
                break;
            }
        }
        // The Scenario setting is not used for sub areas: in the same kingdom the current scenario is kept, from another
        // kingdom its scenario of the save (1 if not reached yet), whatever the scenario Mario is in
        warpToStageSafe(stage, label, 0);
    }

    // Outside (the pipe / door leading there) and inside (where Mario starts), taken in game; else one picture
    void drawPreview(const ImVec2& at) override {
        const char* stage = mKingdom.areas[mIndex].stage;
        char outName[80], inName[80];
        snprintf(outName, sizeof(outName), "%s_out", stage);
        snprintf(inName, sizeof(inName), "%s_in", stage);
        ImTextureID outImage = getSubAreaImage(outName);
        ImTextureID inImage = getSubAreaImage(inName);
        if (!outImage && !inImage) {
            drawStagePreview(at, stage, mKingdom.names[mIndex]);
            return;
        }

        constexpr float cWidth = 384.0f, cHeight = 216.0f, cPad = 6.0f, cText = 34.0f;
        const float boxH = cPad + cText + 2 * (cHeight + cText) + cPad;
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        ImVec2 tl(at.x, std::max(4.0f, std::min(at.y, 900.0f - boxH - 4.0f)));
        drawList->AddRectFilled(tl, ImVec2(tl.x + cWidth + 2 * cPad, tl.y + boxH), IM_COL32(0, 0, 0, 190), 12.0f);
        ImFont* font = ImGui::GetFont();
        float y = tl.y + cPad;
        drawList->AddText(font, 30.0f, ImVec2(tl.x + cPad + 4.0f, y), IM_COL32(255, 255, 255, 255), tr(mKingdom.names[mIndex]));
        y += cText;
        const struct { ImTextureID image; const char* label; } pictures[2] = { { outImage, tr("Entrance") }, { inImage, tr("Inside") } };
        for (const auto& pic : pictures) {
            ImVec2 imgTl(tl.x + cPad, y);
            if (pic.image)
                drawList->AddImage(pic.image, imgTl, ImVec2(imgTl.x + cWidth, imgTl.y + cHeight));
            else
                drawList->AddText(font, 26.0f, ImVec2(imgTl.x + 12.0f, imgTl.y + cHeight / 2 - 13.0f),
                                  IM_COL32(200, 200, 200, 255), tr("No picture"));
            drawList->AddText(font, 26.0f, ImVec2(imgTl.x + 4.0f, imgTl.y + cHeight + 4.0f), IM_COL32(255, 215, 0, 255),
                              pic.label);
            y += cHeight + cText;
        }
    }
};

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::initCategories() {
    if (mCategoriesInitialized) return;
    if (!set) set = SettingsMgr::instance();
    if (!set || !set->getSettings()) return;

    mCategoriesInitialized = true;
    // Older settings could pick a stick to open the menu: fall back to L + DLeft
    if (set->getSettings()->mMenuKey >= IM_ARRAYSIZE(MenuKeys)) set->getSettings()->mMenuKey = 0;

    // 0: Options
    mCategories[0].name = "Options";
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableMoonRefresh, "Moon Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableGrayMoonRefresh, "Gray Moon Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshPurps, "Purple Coins Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDoorRefresh, "Doors Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableShardRefresh, "Moon Shards Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshKingdomEnter, "Kingdom Enter Refresh"));
    // Warp Text Refresh is now part of Cutscene Refresh: an older setting turns Cutscene Refresh on instead
    if (set->getSettings()->mIsEnableRefreshWarpText) {
        set->getSettings()->mIsEnableRefreshWarpText = false;
        set->getSettings()->mIsEnableRefreshNpc = true;
    }
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableFlowerPotRefresh, "Seeds Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSeedGrowing, "Seeds Growing"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableRefreshNpc, "Cutscene Refresh"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysManualCutscene, "Always Manual Cutscene"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysCheckpoints, "Always Checkpoints"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableAutoSave, "Disable Autosave"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableTpPuppet, "Disable TP Puppet"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSkipCloud, "Skip Cloud"));
    // Right column starts here
    mCategories[0].columnSplit = (int)mCategories[0].components.size();
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAllCheckpoints, "All Checkpoints"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableLuigiPixels, "Luigi Pixels"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableHintPhotoSpawn, "Spawn All Hint Art"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnablePipeMazeOverride, "Pipe Maze Override"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNoCheckpointTouch, "No Checkpoint Touch"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNoDamage, "No Damage"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableMusic, "Disable Music"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableDisableMoonLock, "Disable Moon Lock"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSkipBroodals, "Skip Broodals"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableAlwaysOdysseyPowerUp, "Always Odyssey Power Up"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableOdysseyInAllKingdoms, "Odyssey In All Kingdoms"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableNightMetroMultiMoon, "Night Metro Multi Moon"));
    mCategories[0].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableOpenOutfitDoors, "Open Outfit Doors"));
    // Noclip: left stick moves, ZR goes up, ZL goes down, X/Y speed up, A/B slow down (see noclipHook)
    mCategories[0].components.push_back(new BoolMenuComponent(&mIsEnableNoclip, "Noclip"));
    mCategories[0].components.push_back(new BoolMenuComponent(&mIsReloadPos, "Reload Scene at Pos"));

    // Help panel of each option (shown next to the menu while it is selected)
    static const char* const sOptionHelp[][2] = {
        { "Moon Refresh", "Moons you collect still count (moon counter, story) but come back as new moons: you can collect the same moon again as many times as you want." },
        { "Gray Moon Refresh", "Moons already collected look normal again (not gray), as if they were never collected." },
        { "Purple Coins Refresh", "Purple coins come back when the scene is reloaded." },
        { "Doors Refresh", "Doors already opened are closed again when the scene is reloaded." },
        { "Moon Shards Refresh", "Moon shards (and their counter) come back, even when their moon is already collected." },
        { "Kingdom Enter Refresh", "The Odyssey arrival cutscene of the kingdom plays again." },
        { "Seeds Refresh", "Seeds come back and pots are empty at each reload. The plant grows normally (with time or water)." },
        { "Seeds Growing", "A planted seed grows to the end at once and its moon appears. Works with or without Seeds Refresh." },
        { "Cutscene Refresh", "Cutscenes and first talks of the characters (Toads, Peach, Captain Toad...) play again, with Cappy's texts (the checkpoint warp explanation, his texts at the start of the kingdoms) and the camera with the story text at the start of a kingdom." },
        { "Always Manual Cutscene", "Cutscenes never skip by themselves: you choose when to skip them." },
        { "Always Checkpoints", "You can always warp to the flags from the map, even where the game normally forbids it." },
        { "Disable Autosave", "The game no longer saves by itself: your save file stays the same while you practice." },
        { "Disable TP Puppet", "Mario is not placed automatically during the moon get animation." },
        { "Skip Cloud", "In the Cloud Kingdom, Bowser is beaten as soon as his intro cutscene ends: the battle end cutscene plays right away." },
        { "All Checkpoints", "Every flag of the kingdom is available on the map, even the ones not touched yet." },
        { "Luigi Pixels", "The 8-bit Luigi hidden in some bushes always comes out (10% chance in the normal game). Reload the scene after turning it on." },
        { "Spawn All Hint Art", "Every hint art appears, without having to unlock it." },
        { "Pipe Maze Override", "Forces the path of the pipe mazes (the same layout every time)." },
        { "No Checkpoint Touch", "Touching a flag does not activate it (your tries are not changed by a flag touched by mistake)." },
        { "No Damage", "Mario no longer loses health." },
        { "Disable Music", "Turns the music off: only the sound effects stay." },
        { "Disable Moon Lock", "The Odyssey can leave without the number of moons the kingdom normally asks for." },
        { "Skip Broodals", "As soon as the intro of a Broodal fight ends (Hariet, Rango, Spewart, Madame Broode), the boss is beaten and its Multi Moon appears." },
        { "Always Odyssey Power Up", "Each time you go to the Odyssey, even without moons, it powers up and offers the next kingdom of the story." },
        { "Odyssey In All Kingdoms", "The Odyssey is in every kingdom, even the ones your save has not reached yet, and you can warp to it from the map." },
        { "Night Metro Multi Moon", "At the start of Night Metro, the Mechawiggler Multi Moon falls next to Mario: grab it to go straight to day Metro." },
        { "Open Outfit Doors", "The guards of the outfit rooms let Mario in whatever outfit he wears." },
        { "Noclip", "Mario flies through everything. Left stick = move, ZR = up, ZL = down, X/Y = faster, A/B = slower. Turns off at each game launch." },
        { "Reload Scene at Pos", "When the scene is reloaded, Mario comes back exactly where he was." },
    };
    for (MenuComponent* c : mCategories[0].components) {
        if (!c || !c->getWheelName()) continue;
        for (const auto& h : sOptionHelp)
            if (strcmp(h[0], c->getWheelName()) == 0) c->setHelp(h[1]);
    }

    // 1: Stages
    static int sSelectedStageIdx = 0;
    static int sSelectedScenario = 0;
    static const char* sStageNames[17] = {
        "Cap Kingdom", "Cascade Kingdom", "Sand Kingdom", "Wooded Kingdom",
        "Lake Kingdom", "Cloud Kingdom", "Lost Kingdom", "Metro Kingdom",
        "Snow Kingdom", "Seaside Kingdom", "Luncheon Kingdom", "Ruined Kingdom",
        "Bowser's Kingdom", "Moon Kingdom", "Mushroom Kingdom", "Dark Side", "Darker Side"
    };
    mCategories[1].name = "Stages";
    // Same order as stage_warp.cpp mainNames (warpToKingdom), used for the kingdom pictures
    static const char* sKingdomStages[17] = {
        "CapWorldHomeStage", "WaterfallWorldHomeStage", "SandWorldHomeStage", "ForestWorldHomeStage",
        "LakeWorldHomeStage", "CloudWorldHomeStage", "ClashWorldHomeStage", "CityWorldHomeStage",
        "SnowWorldHomeStage", "SeaWorldHomeStage", "LavaWorldHomeStage", "BossRaidWorldHomeStage",
        "SkyWorldHomeStage", "MoonWorldHomeStage", "PeachWorldHomeStage", "Special1WorldHomeStage", "Special2WorldHomeStage"
    };
    mCategories[1].components.push_back(new KingdomMenuComponent(&sSelectedStageIdx, sStageNames, sKingdomStages, 17));
    mCategories[1].components.push_back(new IntMenuComponent<int>(&sSelectedScenario, "Scenario", 0, 15));
    mCategories[1].components.push_back(new ButtonMenuComponent("Warp to Stage", []() {
        warpToKingdom(sSelectedStageIdx, sSelectedScenario);
    }));
    // Sub areas of every kingdom (they use the Scenario above too)
    for (const KingdomSubAreas& kingdom : sKingdomSubAreas)
        mCategories[1].components.push_back(new SubAreaMenuComponent(kingdom, &sSelectedScenario));

    // 2: Misc
    mCategories[2].name = "Misc";
    mCategories[2].components.push_back(new EnumMenuComponent<int>(&mSelectedBody, bodyNames, 42, "Outfit Body"));
    mCategories[2].components.push_back(new EnumMenuComponent<int>(&mSelectedCap, capNames, 42, "Outfit Cap"));
    mCategories[2].components.push_back(new ButtonMenuComponent("Apply Outfit", [this]() {
        applyOutfit();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Save Position", [this]() {
        saveTeleport(tpStates[tpIndex]);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Load Position", [this]() {
        loadTeleport(tpStates[tpIndex]);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Restart Scene", [this]() {
        restartScene();
    }));
    mCategories[2].components.push_back(new BoolMenuComponent(&mIsReloadPos, "Reload at Pos"));
    mCategories[2].components.push_back(new ButtonMenuComponent("Restart at Pos", [this]() {
        triggerReloadPos();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Kill Mario", [this]() {
        if (playerHak) GameDataFunction::killPlayer(GameDataHolderWriter(playerHak));
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Damage Mario", [this]() {
        if (playerHak) {
            bool tmp = set->getSettings()->mIsEnableNoDamage;
            set->getSettings()->mIsEnableNoDamage = false;
            GameDataFunction::damagePlayer(GameDataHolderWriter(playerHak));
            set->getSettings()->mIsEnableNoDamage = tmp;
        }
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Life Up Heart", [this]() {
        if (playerHak) GameDataFunction::getLifeMaxUpItem(playerHak);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Heal Mario", [this]() {
        if (playerHak) GameDataFunction::recoveryPlayer(playerHak);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Add 1000 Coins", [this]() {
        if (stageScene) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), 1000);
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Remove 1000 Coins", [this]() {
        if (stageScene) {
            int coinNum = GameDataFunction::getCoinNum(GameDataHolderAccessor(stageScene));
            if (coinNum < 1000) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -coinNum);
            else GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -1000);
        }
    }));
    // Right column starts here
    mCategories[2].columnSplit = (int)mCategories[2].components.size();
    mCategories[2].components.push_back(new ButtonMenuComponent("Reload Scene", [this]() {
        if (stageScene) stageScene->kill();
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Previous Scene", [this]() {
        if (stageScene) GameDataHolderAccessor(stageScene)->returnPrevStage();
    }));
    mCategories[2].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mWigglerPattern, WigglerPatterns, 23, "Wiggler Pattern"));
    // One custom name for every moon, typed on the on-screen keyboard
    mCategories[2].components.push_back(new ButtonMenuComponent("Rename All Moons", [this]() {
        openKeyboard("Name of every moon", getAllMoonsName(), [this](const char* name) {
            setAllMoonsName(name);
            setPopupText(name[0] ? "Every moon renamed" : "Moon names reset");
        });
    }));
    mCategories[2].components.push_back(new ButtonMenuComponent("Reset All Moon Names", [this]() {
        resetAllMoonNames();
        setPopupText("All moon names reset");
    }));

    // 3: Timer
    mCategories[3].name = "Timer";
    mCategories[3].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSegmentTimer, "Segment Timer"));
    mCategories[3].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSegTimerSplit, "Timer Split"));
    static int sTimerStartTypeIdx = findVisibleTimerHookType(set->getSettings()->mTimerStartType);
    static int sTimerEndTypeIdx = findVisibleTimerHookType(set->getSettings()->mTimerEndType);
    mCategories[3].components.push_back(new TimerTypeMenuComponent(&sTimerStartTypeIdx, VisibleTimerHookTypes, VisibleTimerHookTypeNames, IM_ARRAYSIZE(VisibleTimerHookTypeNames), true, [](int i) {
        SettingsMgr::getSettings()->mTimerStartType = VisibleTimerHookTypes[i];
        if (btt::Timer::sInstance) btt::Timer::sInstance->resetEventCounts();
    }));
    static int sTimerStartCount = set->getSettings()->mTimerStartCount ? set->getSettings()->mTimerStartCount : 1;
    mCategories[3].components.push_back(new TimerCountMenuComponent(&sTimerStartCount, true, [](int n) {
        SettingsMgr::getSettings()->mTimerStartCount = (u8)n;
        if (btt::Timer::sInstance) btt::Timer::sInstance->resetEventCounts();
    }));
    mCategories[3].components.push_back(new TimerTypeMenuComponent(&sTimerEndTypeIdx, VisibleTimerHookTypes, VisibleTimerHookTypeNames, IM_ARRAYSIZE(VisibleTimerHookTypeNames), false, [](int i) {
        SettingsMgr::getSettings()->mTimerEndType = VisibleTimerHookTypes[i];
        if (btt::Timer::sInstance) btt::Timer::sInstance->resetEventCounts();
    }));
    static int sTimerEndCount = set->getSettings()->mTimerEndCount ? set->getSettings()->mTimerEndCount : 1;
    mCategories[3].components.push_back(new TimerCountMenuComponent(&sTimerEndCount, false, [](int n) {
        SettingsMgr::getSettings()->mTimerEndCount = (u8)n;
        if (btt::Timer::sInstance) btt::Timer::sInstance->resetEventCounts();
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Start Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->start();
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Stop Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->stop();
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Reset Timer", []() {
        if (btt::Timer::sInstance) btt::Timer::sInstance->reset();
    }));
    // Splits (like LiveSplit): each End Type event goes to the next split
    mCategories[3].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableSplits, "Splits"));
    mCategories[3].components.back()->setHelp("Each End Type event (after End Count) goes to the next split, the last split ends the run. Under the timer, each split shows the time when it ended.");
    if (set->getSettings()->mSplitCount < 2 || set->getSettings()->mSplitCount > btt::Timer::cMaxSplits) set->getSettings()->mSplitCount = 5;
    mCategories[3].components.push_back(new IntMenuComponent<u8>(&set->getSettings()->mSplitCount, "Split Count", 2, btt::Timer::cMaxSplits, 1));
    // End type of each split: choose the split (Edit Split), then its type (Split Type, "End Type" = the one above)
    constexpr int cTypeNum = IM_ARRAYSIZE(VisibleTimerHookTypeNames);
    static const char* sSplitTypeNames[cTypeNum + 1] = { "End Type" };
    for (int i = 0; i < cTypeNum; i++) sSplitTypeNames[i + 1] = VisibleTimerHookTypeNames[i];
    static int sEditSplit = 1;
    static int sSplitTypeIdx = set->getSettings()->mSplitTypes[0];
    mCategories[3].components.push_back(new IntMenuComponent<int>(&sEditSplit, "Edit Split", 1, btt::Timer::cMaxSplits, 1, 34, [](int n) {
        sSplitTypeIdx = SettingsMgr::getSettings()->mSplitTypes[n - 1];
    }));
    mCategories[3].components.push_back(new EnumMenuComponent<int>(&sSplitTypeIdx, sSplitTypeNames, cTypeNum + 1, "Split Type", 34, [](int i) {
        SettingsMgr::getSettings()->mSplitTypes[sEditSplit - 1] = (u8)i;
    }));
    mCategories[3].components.back()->setHelp("The event that ends the split chosen in Edit Split. \"End Type\": the End Type of the timer.");
    // Right column: position of the timer
    mCategories[3].columnSplit = (int)mCategories[3].components.size();
    static int sTimerPreset = 0;
    static const char* sTimerPresetNames[6] = { "Center", "Top Left", "Top Center", "Top Right", "Bottom Left", "Bottom Right" };
    static int sTimerX = (int)set->getSettings()->mTimerPos.x;
    static int sTimerY = (int)set->getSettings()->mTimerPos.y;
    mCategories[3].components.push_back(new EnumMenuComponent<int>(&sTimerPreset, sTimerPresetNames, 6, "Preset Pos", 34, [](int preset) {
        SettingsMgr* s = SettingsMgr::instance();
        if (!s || !s->getSettings()) return;
        constexpr static ImVec2 sPresets[6] = { { 800.f, 450.f }, { 40.f, 30.f },   { 700.f, 30.f },
                                                { 1380.f, 30.f }, { 40.f, 820.f }, { 1380.f, 820.f } };
        s->getSettings()->mTimerPos = sPresets[preset];
        sTimerX = (int)s->getSettings()->mTimerPos.x;
        sTimerY = (int)s->getSettings()->mTimerPos.y;
    }));
    mCategories[3].components.push_back(new IntMenuComponent<int>(&sTimerX, "Position X", 0, 1600, 25, 34, [](int x) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mTimerPos.x = (float)x;
    }));
    mCategories[3].components.push_back(new IntMenuComponent<int>(&sTimerY, "Position Y", 0, 900, 25, 34, [](int y) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mTimerPos.y = (float)y;
    }));
    mCategories[3].components.push_back(new ButtonMenuComponent("Reset Position", []() {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) {
            s->getSettings()->mTimerPos = ImVec2(800.f, 450.f);
            sTimerPreset = 0;
            sTimerX = 800;
            sTimerY = 450;
        }
    }));

    // 4: Input Display
    static const char* sColorNames[13] = {
        "None", "White", "Gray", "Black", "Red", "Blue", "Yellow", "Green", "Orange", "Purple", "Pink", "LightBlue", "Multicolor"
    };
    static int sDisplayPreset = 0;
    static const char* sPresetNames[5] = { "Center", "Bottom Left", "Bottom Right", "Top Left", "Top Right" };
    static int sInputDisplayX = (int)set->getSettings()->mInputDisplayPos.x;
    static int sInputDisplayY = (int)set->getSettings()->mInputDisplayPos.y;

    mCategories[4].name = "Input Display";
    mCategories[4].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableInputDisplay, "Input Display"));
    mCategories[4].components.push_back(new BoolMenuComponent(&set->getSettings()->mIsEnableInput2P, "2P Mode"));
    // Style chosen before v1.7.1 (212 Open Joystick Display styles, index 3 + OJD number): same style in the new list
    if (set->getSettings()->mInputDisplayStyleList == 0) {
        static constexpr u8 cRemovedOjd[] = { 0, 1, 2, 11, 12, 34, 35, 36, 37, 38, 39, 40, 41, 42, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 129, 130, 131, 132, 133, 134, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211 };
        u8& style = set->getSettings()->mInputDisplayStyle;
        if (style >= 3) {
            int ojd = style - 3, shift = 0;
            for (u8 removed : cRemovedOjd) {
                if (removed == ojd) shift = -1000;
                else if (removed < ojd) shift++;
            }
            style = shift < 0 ? 0 : u8(style - shift);
        }
        set->getSettings()->mInputDisplayStyleList = 1;
    }
    if (set->getSettings()->mInputDisplayStyle >= IM_ARRAYSIZE(sInputDisplayStyleNames)) set->getSettings()->mInputDisplayStyle = 0;
    mCategories[4].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mInputDisplayStyle, sInputDisplayStyleNames, IM_ARRAYSIZE(sInputDisplayStyleNames), "Style"));
    mCategories[4].components.push_back(new EnumMenuComponent<int>(&sDisplayPreset, sPresetNames, 5, "Preset Pos", 34, [](int preset) {
        SettingsMgr* s = SettingsMgr::instance();
        if (!s || !s->getSettings()) return;
        switch (preset) {
            case 0: s->getSettings()->mInputDisplayPos = ImVec2(800.f, 450.f); break; // Center
            case 1: s->getSettings()->mInputDisplayPos = ImVec2(250.f, 720.f); break; // Bottom Left
            case 2: s->getSettings()->mInputDisplayPos = ImVec2(1380.f, 720.f); break; // Bottom Right
            case 3: s->getSettings()->mInputDisplayPos = ImVec2(250.f, 220.f); break; // Top Left
            case 4: s->getSettings()->mInputDisplayPos = ImVec2(1380.f, 220.f); break; // Top Right
        }
        sInputDisplayX = (int)s->getSettings()->mInputDisplayPos.x;
        sInputDisplayY = (int)s->getSettings()->mInputDisplayPos.y;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayX, "Position X", 0, 1600, 25, 34, [](int x) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPos.x = (float)x;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayY, "Position Y", 0, 900, 25, 34, [](int y) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPos.y = (float)y;
    }));
    mCategories[4].components.push_back(new ButtonMenuComponent("Reset Position", []() {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) {
            s->getSettings()->mInputDisplayPos = ImVec2(800.f, 450.f);
            sDisplayPreset = 0;
            sInputDisplayX = 800;
            sInputDisplayY = 450;
        }
    }));
    if (set->getSettings()->mInputDisplaySize < 30 || set->getSettings()->mInputDisplaySize > 150)
        set->getSettings()->mInputDisplaySize = 100;
    mCategories[4].components.push_back(new IntMenuComponent<int>(&set->getSettings()->mInputDisplaySize, "Size", 30, 150, 10));
    // Right column: position of the 2nd controller (2P Mode), then the colors
    mCategories[4].columnSplit = (int)mCategories[4].components.size();
    static int sDisplayPresetP2 = 0;
    static int sInputDisplayP2X = (int)set->getSettings()->mInputDisplayPosP2.x;
    static int sInputDisplayP2Y = (int)set->getSettings()->mInputDisplayPosP2.y;
    mCategories[4].components.push_back(new EnumMenuComponent<int>(&sDisplayPresetP2, sPresetNames, 5, "P2 Preset Pos", 34, [](int preset) {
        SettingsMgr* s = SettingsMgr::instance();
        if (!s || !s->getSettings()) return;
        switch (preset) {
            case 0: s->getSettings()->mInputDisplayPosP2 = ImVec2(800.f, 450.f); break; // Center
            case 1: s->getSettings()->mInputDisplayPosP2 = ImVec2(250.f, 720.f); break; // Bottom Left
            case 2: s->getSettings()->mInputDisplayPosP2 = ImVec2(1380.f, 720.f); break; // Bottom Right
            case 3: s->getSettings()->mInputDisplayPosP2 = ImVec2(250.f, 220.f); break; // Top Left
            case 4: s->getSettings()->mInputDisplayPosP2 = ImVec2(1380.f, 220.f); break; // Top Right
        }
        sInputDisplayP2X = (int)s->getSettings()->mInputDisplayPosP2.x;
        sInputDisplayP2Y = (int)s->getSettings()->mInputDisplayPosP2.y;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayP2X, "P2 Position X", 0, 1600, 25, 34, [](int x) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPosP2.x = (float)x;
    }));
    mCategories[4].components.push_back(new IntMenuComponent<int>(&sInputDisplayP2Y, "P2 Position Y", 0, 900, 25, 34, [](int y) {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) s->getSettings()->mInputDisplayPosP2.y = (float)y;
    }));
    mCategories[4].components.push_back(new ButtonMenuComponent("P2 Reset Position", []() {
        SettingsMgr* s = SettingsMgr::instance();
        if (s && s->getSettings()) {
            s->getSettings()->mInputDisplayPosP2 = ImVec2(800.f, 450.f);
            sDisplayPresetP2 = 0;
            sInputDisplayP2X = 800;
            sInputDisplayP2Y = 450;
        }
    }));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayButtonColor, sColorNames, 13, "Button Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayButtonPressedColor, sColorNames, 13, "Pressed Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayStickColor, sColorNames, 13, "Stick Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayRingColor, sColorNames, 13, "Ring Color"));
    mCategories[4].components.push_back(new EnumMenuComponent<SettingsMgr::InputDisplayColor>(&set->getSettings()->mInputDisplayBackColor, sColorNames, 13, "BG Color"));

    // Quick wheel actions: the Options, Misc and Timer entries built above
    buildWheelActions(this);

    // 5: Keybinds
    mCategories[5].name = "Keybinds";
    // Every keybind: the usual choices, or "Custom" (A records any combination of buttons, menu/CustomKeys.h)
    using TpKey = SettingsMgr::TpKey;
    using Hotkey = SettingsMgr::SettingsHotkey;
    auto* st = set->getSettings();
    mCategories[5].components.push_back(new KeyBindMenuComponent<u8>(&st->mMenuKey, MenuKeys, IM_ARRAYSIZE(MenuKeys), "Open Menu", 4, OpenMenu));
    mCategories[5].components.push_back(new KeyBindMenuComponent<TpKey>(&st->mSavePosKey, TpKeys, IM_ARRAYSIZE(TpKeys), "Save Position", int(TpKey::Custom), SavePos));
    mCategories[5].components.push_back(new KeyBindMenuComponent<TpKey>(&st->mLoadPosKey, TpKeys, IM_ARRAYSIZE(TpKeys), "Load Position", int(TpKey::Custom), LoadPos));
    mCategories[5].components.push_back(new KeyBindMenuComponent<TpKey>(&st->mRestartSceneKey, TpKeys, IM_ARRAYSIZE(TpKeys), "Restart Scene", int(TpKey::Custom), RestartScene));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mHealMarioKey, Keys, IM_ARRAYSIZE(Keys), "Heal Mario", int(Hotkey::Custom), HealMario));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mLifeUpKey, Keys, IM_ARRAYSIZE(Keys), "Life Up", int(Hotkey::Custom), LifeUp));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mPrevSceneKey, Keys, IM_ARRAYSIZE(Keys), "Prev Scene", int(Hotkey::Custom), PrevScene));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mWarpLastCpKey, Keys, IM_ARRAYSIZE(Keys), "Warp Last CP", int(Hotkey::Custom), WarpLastCp));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mTimerStartKey, Keys, IM_ARRAYSIZE(Keys), "Timer Start", int(Hotkey::Custom), TimerStart));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mTimerEndKey, Keys, IM_ARRAYSIZE(Keys), "Timer End", int(Hotkey::Custom), TimerEnd));
    mCategories[5].components.push_back(new KeyBindMenuComponent<Hotkey>(&st->mTimerResetKey, Keys, IM_ARRAYSIZE(Keys), "Timer Reset", int(Hotkey::Custom), TimerReset));
    // Right column: the quick wheel (menu/QuickWheel.h)
    mCategories[5].columnSplit = (int)mCategories[5].components.size();
    if (set->getSettings()->mWheelKey >= IM_ARRAYSIZE(cWheelKeyNames)) set->getSettings()->mWheelKey = 0;
    mCategories[5].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mWheelKey, cWheelKeyNames, IM_ARRAYSIZE(cWheelKeyNames), "Wheel Key"));
    if (set->getSettings()->mWheelSize >= IM_ARRAYSIZE(cWheelSizeNames)) set->getSettings()->mWheelSize = 1;
    mCategories[5].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mWheelSize, cWheelSizeNames, IM_ARRAYSIZE(cWheelSizeNames), "Wheel Size"));
    static const char* const sWheelSlotTitles[cWheelSlotNum] = {
        "Wheel Slot 1", "Wheel Slot 2", "Wheel Slot 3", "Wheel Slot 4",  "Wheel Slot 5",  "Wheel Slot 6",
        "Wheel Slot 7", "Wheel Slot 8", "Wheel Slot 9", "Wheel Slot 10", "Wheel Slot 11", "Wheel Slot 12",
    };
    static int sWheelSlotActions[cWheelSlotNum] = {};
    for (int i = 0; i < cWheelSlotNum; i++) {
        sWheelSlotActions[i] = findWheelAction(set->getSettings()->mWheelSlots[i]);
        mCategories[5].components.push_back(new EnumMenuComponent<int>(&sWheelSlotActions[i], getWheelActionNames(), getWheelActionNum(), sWheelSlotTitles[i], 34, [i](int action) {
            SettingsMgr::Settings* s = SettingsMgr::getSettings();
            if (!s) return;
            const char* name = action > 0 ? getWheelActionNames()[action] : "";
            snprintf(s->mWheelSlots[i], sizeof(s->mWheelSlots[i]), "%s", name);
        }));
    }

    // 6: Info
    mCategories[6].name = "Info";
    mCategories[6].components.push_back(new BoolMenuComponent(&isEnableInfoWindow, "Info Window"));
    mCategories[6].components.push_back(new BoolMenuComponent(&isEnablePlayerInfo, "Player Info"));

    // 7: Settings
    mCategories[7].name = "Settings";
    mCategories[7].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mMenuCorner, Corners, 4, "Menu Corner"));
    if (set->getSettings()->mLanguage >= IM_ARRAYSIZE(cLanguageNames)) set->getSettings()->mLanguage = 0;
    mCategories[7].components.push_back(new EnumMenuComponent<u8>(&set->getSettings()->mLanguage, cLanguageNames, IM_ARRAYSIZE(cLanguageNames), "Language"));
}

int Menu::getCategoryHeight(int id) {
    int height = 0;
    ImFont* font = ImGui::GetIO().Fonts->Fonts[0];
    for (int i = 0; i < id; i++) {
        ImVec2 textSize = font ? font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, tr(mCategories[i].name)) : ImVec2(100, (float)sCategoryFontSize);
        height += sCategorySpacing + (int)textSize.y + sCategorySpacing;
    }
    return height;
}

void Menu::draw() {
    if (!set) set = SettingsMgr::instance();
    initCategories();

    if (!mIsEnabledMenu) return;

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->Fonts[0];
    if (!font) return;

    mBgSize = ImVec2(0, 0);
    for (int i = 0; i < 8; i++) {
        ImVec2 textSize = font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, tr(mCategories[i].name));
        float width = (float)sCategorySpacing * 2.0f + textSize.x + (float)sCategorySpacing * 2.0f;
        mBgSize.x = std::max(mBgSize.x, width);
    }
    mBgSize.y = (float)getCategoryHeight(8);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // 1. Sidebar background: translucent dark bar
    drawList->AddRectFilled(ImVec2(0, 0), mBgSize, IM_COL32(0, 0, 0, 140));

    // 2. Highlight for selected category
    drawList->AddRectFilled(
        ImVec2(0, (float)getCategoryHeight(mCurrentCategory)),
        ImVec2(mBgSize.x, (float)getCategoryHeight(mCurrentCategory + 1)),
        IM_COL32(255, 255, 255, 76)
    );

    // 3. Category labels in white
    float curY = 0;
    for (int i = 0; i < 8; i++) {
        ImVec2 textSize = font->CalcTextSizeA((float)sCategoryFontSize, FLT_MAX, 0.f, tr(mCategories[i].name));
        curY += (float)sCategorySpacing;
        drawList->AddText(font, (float)sCategoryFontSize, ImVec2((float)sCategorySpacing * 2.0f, curY), IM_COL32(255, 255, 255, 255), tr(mCategories[i].name));
        curY += textSize.y + (float)sCategorySpacing;
    }

    // 4. Expanded drawer for current category items
    if (mIsExpandedCurrentCategory) {
        drawExpandedCategory();
    }
    drawKeyboard();
}

void Menu::drawExpandedCategory() {
    auto& cat = mCategories[mCurrentCategory];
    if (cat.components.empty()) return;

    if (mCurrentComponentInCategory >= (int)cat.components.size())
        mCurrentComponentInCategory = (int)cat.components.size() - 1;
    if (mCurrentComponentInCategory < 0)
        mCurrentComponentInCategory = 0;

    if (cat.columnSplit > 0 && cat.columnSplit < (int)cat.components.size()) {
        drawTwoColumnCategory(cat);
        return;
    }

    float subX = 0;
    for (int i = 0; i < (int)cat.components.size(); i++) {
        MenuComponent* comp = cat.components[i];
        if (!comp) continue;
        ImVec2 size = comp->getSize();
        subX = std::max(subX, size.x);
    }

    int maxVisible = 14;
    int scrollOffset = 0;
    if ((int)cat.components.size() > maxVisible) {
        if (mCurrentComponentInCategory >= maxVisible - 2) {
            scrollOffset = mCurrentComponentInCategory - maxVisible + 3;
        }
        if (scrollOffset + maxVisible > (int)cat.components.size()) {
            scrollOffset = (int)cat.components.size() - maxVisible;
        }
        if (scrollOffset < 0) scrollOffset = 0;
    }
    int endIndex = std::min((int)cat.components.size(), scrollOffset + maxVisible);

    float startY = (float)getCategoryHeight(mCurrentCategory);
    float totalDrawHeight = (endIndex - scrollOffset) * 44.0f;
    if (startY + totalDrawHeight > 700.0f) {
        startY = std::max(10.0f, 700.0f - totalDrawHeight);
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    float drawY = 0;
    ImVec2 previewAt(0, 0);
    for (int i = scrollOffset; i < endIndex; i++) {
        MenuComponent* comp = cat.components[i];
        if (!comp) continue;
        ImVec2 size = comp->getSize();
        float itemH = size.y > 0 ? size.y : 40.0f;

        // Card background (translucent dark rounded rectangle)
        drawList->AddRectFilled(
            ImVec2(mBgSize.x + 8.0f, startY + drawY),
            ImVec2(mBgSize.x + 8.0f + subX, startY + drawY + itemH),
            IM_COL32(0, 0, 0, 150),
            12.0f
        );

        // Highlight for selected component
        if (mCurrentComponentInCategory == i) {
            ImU32 highlightCol = mIsFocusedOnCurrentComponent ? IM_COL32(255, 60, 60, 100) : IM_COL32(255, 255, 255, 76);
            drawList->AddRectFilled(
                ImVec2(mBgSize.x + 8.0f, startY + drawY),
                ImVec2(mBgSize.x + 8.0f + subX, startY + drawY + itemH),
                highlightCol,
                12.0f
            );
        }

        comp->setIsHovered(mCurrentComponentInCategory == i);
        comp->setIsFocused(mCurrentComponentInCategory == i && mIsFocusedOnCurrentComponent);
        comp->draw(ImVec2(mBgSize.x + 8.0f, startY + drawY));
        if (mCurrentComponentInCategory == i) previewAt = ImVec2(mBgSize.x + 8.0f + subX + 12.0f, startY + drawY);

        drawY += itemH + 3.0f;
    }
    if (mCurrentComponentInCategory >= scrollOffset && mCurrentComponentInCategory < endIndex)
        cat.components[mCurrentComponentInCategory]->drawPreview(previewAt);
}

void Menu::drawTwoColumnCategory(Category& cat) {
    const int count = (int)cat.components.size();
    const int split = cat.columnSplit;
    const int ranges[2][2] = {{0, split}, {split, count}};

    float colWidth[2] = {0, 0};
    for (int c = 0; c < 2; c++) {
        for (int i = ranges[c][0]; i < ranges[c][1]; i++) {
            if (cat.components[i]) colWidth[c] = std::max(colWidth[c], cat.components[i]->getSize().x);
        }
    }

    int maxRows = std::max(split, count - split);
    float startY = (float)getCategoryHeight(mCurrentCategory);
    float totalDrawHeight = maxRows * 44.0f;
    if (startY + totalDrawHeight > 700.0f) {
        startY = std::max(10.0f, 700.0f - totalDrawHeight);
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    float colX = mBgSize.x + 8.0f;
    float previewY = startY;
    for (int c = 0; c < 2; c++) {
        float drawY = 0;
        for (int i = ranges[c][0]; i < ranges[c][1]; i++) {
            MenuComponent* comp = cat.components[i];
            if (!comp) continue;
            ImVec2 size = comp->getSize();
            float itemH = size.y > 0 ? size.y : 40.0f;
            ImVec2 tl(colX, startY + drawY);
            ImVec2 br(colX + colWidth[c], startY + drawY + itemH);

            drawList->AddRectFilled(tl, br, IM_COL32(0, 0, 0, 150), 12.0f);
            if (mCurrentComponentInCategory == i) {
                ImU32 highlightCol = mIsFocusedOnCurrentComponent ? IM_COL32(255, 60, 60, 100) : IM_COL32(255, 255, 255, 76);
                drawList->AddRectFilled(tl, br, highlightCol, 12.0f);
            }

            comp->setIsHovered(mCurrentComponentInCategory == i);
            comp->setIsFocused(mCurrentComponentInCategory == i && mIsFocusedOnCurrentComponent);
            comp->draw(tl);
            if (mCurrentComponentInCategory == i) previewY = tl.y;

            drawY += itemH + 3.0f;
        }
        colX += colWidth[c] + 8.0f;
    }
    // Help of the selected entry, right of both columns
    if (mCurrentComponentInCategory >= 0 && mCurrentComponentInCategory < count && cat.components[mCurrentComponentInCategory])
        cat.components[mCurrentComponentInCategory]->drawPreview(ImVec2(colX + 4.0f, previewY));
}

static bool isPressMenuKey() {
    if (InputHelper::isHoldL() && InputHelper::isPressPadLeft()) return true;
    switch (SettingsMgr::getSettings()->mMenuKey) {
    case 1: return InputHelper::isHoldL() && InputHelper::isPressPadRight();
    case 2: return InputHelper::isHoldR() && InputHelper::isPressPadLeft();
    case 3: return InputHelper::isHoldR() && InputHelper::isPressPadRight();
    case 4: {
        // In the menu, a combination made only of the buttons that move in it (A, B, D-Pad) does not close it, or the
        // menu could not be used any more (L + D-Pad Left always closes it)
        constexpr uint16_t cNavigationButtons = (1 << 0) | (1 << 1) | (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15);
        if (Menu::instance() && Menu::instance()->mIsEnabledMenu && !(getCustomKey(OpenMenu) & ~cNavigationButtons))
            return false;
        return isCustomKeyPressed(OpenMenu);
    }
    default: return false;
    }
}

void Menu::updateMenuInput() {
    // The on-screen keyboard takes every input while it is open
    if (isKeyboardOpen()) {
        updateKeyboardInput();
        return;
    }
    // Keybinds > Custom: the buttons being recorded do not move in the menu
    if (updateCustomKeyCapture()) return;
    if (isPressMenuKey()) {
        if (mIsEnabledMenu) hideMenu();
        else showMenu();
        return;
    }

    if (!mIsEnabledMenu) return;

    auto& cat = mCategories[mCurrentCategory];

    if (!mIsExpandedCurrentCategory) {
        if (InputHelper::isPressPadDown()) {
            mCurrentCategory = (mCurrentCategory + 1) % 8;
        } else if (InputHelper::isPressPadUp()) {
            mCurrentCategory = (mCurrentCategory + 7) % 8;
        }

        if (InputHelper::isPressPadRight() || InputHelper::isPressA()) {
            mIsExpandedCurrentCategory = true;
            mCurrentComponentInCategory = 0;
            mIsFocusedOnCurrentComponent = false;
        }
    } else {
        const int count = (int)cat.components.size();
        const bool twoColumns = cat.columnSplit > 0 && cat.columnSplit < count;
        const bool inRightColumn = twoColumns && mCurrentComponentInCategory >= cat.columnSplit;

        if (!mIsFocusedOnCurrentComponent && twoColumns && (InputHelper::isPressPadDown() || InputHelper::isPressPadUp() ||
                                                            InputHelper::isPressPadRight() || (inRightColumn && InputHelper::isPressPadLeft()))) {
            // Two-column navigation: up/down wrap inside the current column, left/right switch column on the same row
            const int colStart = inRightColumn ? cat.columnSplit : 0;
            const int colSize = inRightColumn ? count - cat.columnSplit : cat.columnSplit;
            const int row = mCurrentComponentInCategory - colStart;
            if (InputHelper::isPressPadDown()) {
                mCurrentComponentInCategory = colStart + (row + 1) % colSize;
            } else if (InputHelper::isPressPadUp()) {
                mCurrentComponentInCategory = colStart + (row + colSize - 1) % colSize;
            } else if (InputHelper::isPressPadRight()) {
                if (!inRightColumn)
                    mCurrentComponentInCategory = cat.columnSplit + std::min(row, count - cat.columnSplit - 1);
            } else {
                mCurrentComponentInCategory = std::min(row, cat.columnSplit - 1);
            }
        } else if (!mIsFocusedOnCurrentComponent) {
            if (InputHelper::isPressPadDown()) {
                if (!cat.components.empty()) {
                    mCurrentComponentInCategory = (mCurrentComponentInCategory + 1) % cat.components.size();
                }
            } else if (InputHelper::isPressPadUp()) {
                if (!cat.components.empty()) {
                    mCurrentComponentInCategory = (mCurrentComponentInCategory + (int)cat.components.size() - 1) % cat.components.size();
                }
            } else if (InputHelper::isPressPadLeft() || InputHelper::isPressB()) {
                mIsExpandedCurrentCategory = false;
            } else if (InputHelper::isPressA()) {
                if (mCurrentComponentInCategory >= 0 && mCurrentComponentInCategory < (int)cat.components.size()) {
                    auto* comp = cat.components[mCurrentComponentInCategory];
                    if (comp->canHaveFocus()) {
                        mIsFocusedOnCurrentComponent = true;
                    } else {
                        comp->onAction();
                    }
                }
            }
        } else {
            if (mCurrentComponentInCategory >= 0 && mCurrentComponentInCategory < (int)cat.components.size()) {
                auto* comp = cat.components[mCurrentComponentInCategory];
                if (InputHelper::isPressPadLeft()) {
                    comp->onLeft();
                } else if (InputHelper::isPressPadRight()) {
                    comp->onRight();
                } else if (InputHelper::isPressA()) {
                    mIsFocusedOnCurrentComponent = false;
                    comp->onConfirm();
                } else if (InputHelper::isPressB()) {
                    mIsFocusedOnCurrentComponent = false;
                }
            }
        }
    }
}

void Menu::handleAlways() {
    set = SettingsMgr::instance();
    gameSeq = helpers::tryGetHakoniwaSequence();
    stageScene = helpers::tryGetStageScene(gameSeq);
    if (stageScene) {
        if (strcmp(
                GameDataFunction::getCurrentStageName(gameSeq->mGameDataHolderAccessor), "ClashWorldHomeStage"
            ) == 0)
            noGetPlayer = false;
    }
    if (noGetPlayer) {
        playerHak = nullptr;
    } else {
        playerHak = helpers::tryGetPlayerActorHakoniwa(gameSeq);
    }

    menuTimer++;

    updateMenuInput();

    updateQuickWheel(this);
    if (!isQuickWheelBlockingHotkeys()) handleHotkeys();

    updateStageWarp();
    updatePhotoTour();
    updateDebugCommands();

    if (mCancelCameraInterpoleFrames > 0) {
        mCancelCameraInterpoleFrames--;
        if (playerHak) {
            al::LiveActor* hack = playerHak->mHackKeeper ? playerHak->mHackKeeper->mHackActor : nullptr;
            al::LiveActor* target = hack ? hack : (al::LiveActor*)playerHak;
            al::requestCancelCameraInterpole(target, 0);
        }
        if (stageScene) {
            al::requestCancelCameraInterpole(stageScene, 0);
        }
    }

    if (mReloadState == 1) {
        // Step 1: Wait for old scene to die or become invalid
        if (!stageScene || stageScene != mReloadOldScene || !stageScene->mIsAlive) {
            mReloadState = 2;
            reloadPosTimer = 0;
        }
    } else if (mReloadState == 2) {
        // Step 2: New scene has loaded, wait for player to initialize
        if (stageScene && stageScene != mReloadOldScene && stageScene->mIsAlive && playerHak) {
            reloadPosTimer++;
            if (reloadPosTimer >= 10) {
                playerHak->startDemoPuppetable();
                al::setTrans(playerHak, reloadStagePos);
                al::updatePoseQuat(playerHak, reloadStageQuat);
                al::setVelocityZero(playerHak);
                playerHak->endDemoPuppetable();
                al::requestCancelCameraInterpole(playerHak, 0);
                if (stageScene) al::requestCancelCameraInterpole(stageScene, 0);
                mCancelCameraInterpoleFrames = 10;
                menuTimer = 0;
                mReloadState = 0;
                reloadPosTimer = -1;
                mReloadOldScene = nullptr;
                setPopupText("Reloaded at Position");
            }
        }
    } else if (reloadPosTimer != -1 && playerHak) {
        // Safety fallback
        reloadPosTimer++;
        if (reloadPosTimer >= 15) {
            playerHak->startDemoPuppetable();
            al::setTrans(playerHak, reloadStagePos);
            al::updatePoseQuat(playerHak, reloadStageQuat);
            al::setVelocityZero(playerHak);
            playerHak->endDemoPuppetable();
            al::requestCancelCameraInterpole(playerHak, 0);
            menuTimer = 0;
            reloadPosTimer = -1;
            setPopupText("Reloaded at Position");
        }
    }
}

void Menu::hideMenu() {
    mIsEnabledMenu = false;
    mIsExpandedCurrentCategory = false;
    mIsFocusedOnCurrentComponent = false;
    prevMouseDis = InputHelper::isDisableMouse();
    InputHelper::setDisableMouse(true);
}

void Menu::showMenu() {
    mIsEnabledMenu = true;
    mIsExpandedCurrentCategory = false;
    mIsFocusedOnCurrentComponent = false;
    menuTimer = 0;
    InputHelper::setDisableMouse(prevMouseDis);
}

void Menu::drawPopup() {
    if (!mIsPopup) return;
    if (++popupTimer > 120) {
        mIsPopup = false;
        popupTimer = 0;
        return;
    }
    ImGui::Begin(
        "Popup", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoNavFocus
    );
    ImGui::SetWindowSize(ImVec2(0, 0));
    ImGui::SetWindowPos(ImVec2(mWindowSize.x + 10, 0));
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("%s", popupText);
    ImGui::End();
}

const char* Menu::getMoonRefreshText() {
    if (strcmp(MoonRefreshTexts[set->getSettings()->mMoonRefreshText], "<blank>") != 0) {
        return MoonRefreshTexts[set->getSettings()->mMoonRefreshText];
    } else {
        return "";
    }
}

ImVec2 Menu::getCornerPos(int corner) {
    ImVec2 pos = ImVec2(0, 0);
    nn::oe::OperationMode mode = nn::oe::GetOperationMode();
    if (mode == nn::oe::OperationMode_Docked) {
        switch (corner) {
        case 0: pos = ImVec2(0, 0); break;
        case 1: pos = ImVec2(1600 - mWindowSize.x, 0); break;
        case 2: pos = ImVec2(0, 900 - mWindowSize.y); break;
        case 3: pos = ImVec2(1600 - mWindowSize.x, 900 - mWindowSize.y); break;
        }
    } else {
        switch (corner) {
        case 0: pos = ImVec2(0, 0); break;
        case 1: pos = ImVec2(1280 - mWindowSize.x, 0); break;
        case 2: pos = ImVec2(0, 720 - mWindowSize.y); break;
        case 3: pos = ImVec2(1280 - mWindowSize.x, 720 - mWindowSize.y); break;
        }
    }
    return pos;
}

void Menu::applyOutfit() {
    GameDataHolder* holder = nullptr;
    if (stageScene) {
        holder = GameDataHolderAccessor(stageScene).mData;
    } else if (gameSeq) {
        holder = gameSeq->mGameDataHolderAccessor.mData;
    }

    if (holder) {
        GameDataHolderWriter writer(holder);
        if (mSelectedCap >= 0 && mSelectedCap < 42) {
            GameDataFunction::wearCap(writer, capNames[mSelectedCap]);
        }
        if (mSelectedBody >= 0 && mSelectedBody < 42) {
            GameDataFunction::wearCostume(writer, bodyNames[mSelectedBody]);
        }
        if (stageScene) {
            stageScene->kill();
        }
        setPopupText("Outfit Applied");
    }
}

} // namespace btt
