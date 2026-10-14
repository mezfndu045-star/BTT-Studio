#include "menu/stage_warp.h"

#include "al/Library/Base/StringUtil.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"

#include <cstddef>
#include <cstdio>

#include "helpers/getHelper.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "menu/SubAreaList.h"

struct KingdomEnglishNameSub {
    const char* mInternal;
    const char* mEnglish;
};

struct KingdomEnglishNameMain {
    const char* mInternal;
    const char* mEnglish;
    const int clearMainScenario;
    const int endingScenario;
    const int moonRockScenario;
    const KingdomEnglishNameSub* mSubNames;
    const size_t mSubNamesCount;
};

bool isShowMenu = false;
s32 curScenario = 0;

KingdomEnglishNameSub subNamesCap[] = {
    { "FrogSearchExStage", "Frog Pond" },       { "CapWorldTowerStage", "Inside Cap Tower" },
    { "PoisonWaveExStage", "Poison Tides" },    { "RollingExStage", "Precision Rolling" },
    { "PushBlockExStage", "Push Block Peril" },
};
KingdomEnglishNameSub subNamesCascade[] = {
    { "Lift2DExStage", "8-Bit Chasm Lifts" },
    { "TrexPoppunExStage", "Dinosaur Nest" },
    { "WindBlowExStage", "Gusty Bridges" },
    { "CapAppearExStage", "Mysterious Clouds" },
    { "WanwanClashExStage", "Nice Shots with Chain Chomps" },
};
KingdomEnglishNameSub subNamesSand[] = {
    { "SandWorldKillerExStage", "Bullet Bill Maze" },
    { "RocketFlowerExStage", "Colossal Ruins" },
    { "SandWorldCostumeStage", "Costume Room" },
    { "SandWorldShopStage", "Crazy Cap Store" },
    { "SandWorldUnderground001Stage", "Deepest Underground" },
    { "WaterTubeExStage", "Freezing Waterway" },
    { "SandWorldPressExStage", "Ice Cave" },
    { "SandWorldPyramid000Stage", "Inverted Pyramid: Lower Interior" },
    { "SandWorldPyramid001Stage", "Inverted Pyramid: Upper Interior" },
    { "SandWorldSphinxExStage", "Jaxi Driving" },
    { "SandWorldVibrationStage", "Rumbling Floor House" },
    { "SandWorldSecretStage", "Sphynx Treasure Vault" },
    { "SandWorldRotateExStage", "Strange Neighborhood" },
    { "MeganeLiftExStage", "The Invisible Maze" },
    { "SandWorldSlotStage", "Tostarena Slots" },
    { "SandWorldMeganeExStage", "Transparent Lifts" },
    { "SandWorldUnderground000Stage", "Underground Ruins" },
};
KingdomEnglishNameSub subNamesWooded[] = {
    { "KillerRoadExStage", "Breakdown Road" },
    { "ForestWorldWoodsCostumeStage", "Costume Room" },
    { "ShootingElevatorExStage", "Crowded Elevator" },
    { "ForestWorldWoodsStage", "Deep Woods" },
    { "ForestWorldWoodsTreasureStage", "Deep Woods Treasure Vault" },
    { "ForestWorldWaterExStage", "Flooding Pipeway" },
    { "RailCollisionExStage", "Flower Road" },
    { "AnimalChaseExStage", "Herding Sheep" },
    { "PackunPoisonExStage", "Invisible Road" },
    { "ForestWorldBossStage", "Secret Flower Field" },
    { "FogMountainExStage", "Shards in the Fog" },
    { "ForestWorldTowerStage", "Sky Garden Tower" },
    { "ForestWorldBonusStage", "Spinning-Platforms Treasure Vault" },
    { "ForestWorldCloudBonusExStage", "Walking on Clouds" },
};
KingdomEnglishNameSub subNamesLake[] = {
    { "LakeWorldShopStage", "Crazy Cap Store" }, { "TrampolineWallCatchExStage", "Jump, Grab, Cling, Climb" },
    { "GotogotonExStage", "Puzzle Room" },       { "FastenerExStage", "Unzipping the Chasm" },
    { "FrogPoisonExStage", "Waves of Poison" },
};
KingdomEnglishNameSub subNamesCloud[] = {
    { "Cube2DExStage", "2D Cube" },
    { "FukuwaraiKuriboStage", "Picture Match" },
};
KingdomEnglishNameSub subNamesLost[] = {
    { "ClashWorldShopStage", "Crazy Cap Store" },
    { "ImomuPoisonExStage", "Stretch and Traverse the Jungle" },
    { "JangoExStage", "Klepto 2 - Lava Boogaloo" },
};
KingdomEnglishNameSub subNamesMetro[] = {
    { "PoleKillerExStage", "Bullet Billding" },
    { "CityPeopleRoadStage", "Crowded Alleyway" },
    { "CityWorldShop01Stage", "Crazy Cap Store" },
    { "CityWorldMainTowerStage", "Inside New Donk City Tower" },
    { "CityWorldSandSlotStage", "Metro Kingdom Slots" },
    { "Theater2DExStage", "On the Big Screen" },
    { "DonsukeExStage", "Pitchblack Mountain" },
    { "Note2D3DRoomExStage", "Rainbow Notes" },
    { "RadioControlExStage", "RC Race" },
    { "ElectricWireExStage", "Rewiring the Neighborhood" },
    { "CapRotatePackunExStage", "Rotating Maze Shards" },
    { "BikeSteelExStage", "Scooter Daredevil" },
    { "TrexBikeExStage", "Scooter Escape!" },
    { "CityWorldFactoryStage", "Sewers" },
    { "ShootingCityExStage", "Shards Under Siege" },
    { "PoleGrabCeilExStage", "Swinging Along the High-Rises" },
    { "SwingSteelExStage", "Swinging Scaffolding" },

};
KingdomEnglishNameSub subNamesSnow[] = {
    { "SnowWorldLobby000Stage", "Class A Lobby" },
    { "SnowWorldLobbyExStage", "Class S Lobby" },
    { "SnowWorldCostumeStage", "Cold Room" },
    { "SnowWorldShopStage", "Crazy Cap Store" },
    { "IceWaterDashExStage", "Dashing Over Cold Water" },
    { "IceWaterBlockExStage", "Freezing Water" },
    { "IceWalkerExStage", "Shards in the Cold Room" },
    { "SnowWorldTownStage", "Shiveria Town" },
    { "SnowWorldCloudBonusExStage", "Spinning Above the Clouds" },
    { "ByugoPuzzleExStage", "Ty-Foo Sliding Puzzle" },
    { "KillerRailCollisionExStage", "Wintery Flower Road" },
};
KingdomEnglishNameSub subNamesSeaside[] = {
    { "CloudExStage", "A Sea of Clouds" },
    { "SeaWorldCostumeStage", "Costume Room" },
    { "WaterValleyExStage", "Flying Through the Narrow Valley" },
    { "SeaWorldVibrationStage", "Rumbling Floor Cave" },
    { "SeaWorldSecretStage", "Sphynx Treasure Vault" },
    { "TogezoRotateExStage", "Spinning Maze" },
    { "ReflectBombExStage", "Stop, Poke, and Roll" },
    { "SenobiTowerExStage", "Stretching Up the Sinking Island" },
    { "SeaWorldUtsuboCaveStage", "Underwater Tunnel" },
    { "SeaWorldSneakingManStage", "Wriggling Power Moon" },

};
KingdomEnglishNameSub subNamesLuncheon[] = {
    { "GabuzouClockExStage", "Blazing Above the Gears" },
    { "LavaWorldShopStage", "Crazy Cap Store(?)" },
    { "ForkExStage", "Fork-Flickin to the Summit" },
    { "LavaWorldFenceLiftExStage", "Lava Island Fly-By" },
    { "LavaBonus1Zone", "Luncheon Kingdom Slots(?)" },
    { "LavaWorldTreasureStage", "Luncheon Treasure Vault(?)" },
    { "LavaWorldUpDownExStage", "Magma Swamp" },
    { "LavaWorldBubbleLaneExStage", "Narrow Magma Path" },
    { "LavaWorldExcavationExStage", "Shards in the Cheese Rocks" },
    { "LavaWorldCostumeStage", "Simmering in the Kitchen" },
    { "LavaWorldClockExStage", "Spinning Athletics" },
    { "CapAppearLavaLiftExStage", "Volcano Cave Cruising" },
};
KingdomEnglishNameSub subNamesRuined[] = {
    { "AttackWorldHomeStage", "Ruined Kingdom" },
    { "BullRunExStage", "Mummy Army" },
    { "DotTowerExStage", "Roulette Tower" },
};
KingdomEnglishNameSub subNamesBowser[] = {
    { "SkyWorldShopStage", "Crazy Cap Store" },
    { "SkyWorldTreasureStage", "Bowser's Kingdom Treasure Vault" },
    { "SkyWorldCloudBonusExStage", "Dashing Above the Clouds" },
    { "KaronWingTowerStage", "Hexagon Tower" },
    { "JizoSwitchExStage", "Jizo's Great Adventure" },
    { "SkyWorldCostumeStage", "Scene in the Folding Screen" },
    { "TsukkunRotateExStage", "Spinning Tower" },
    { "TsukkunClimbExStage", "Wooden Tower" },
};
KingdomEnglishNameSub subNamesMoon[] = {
    { "Galaxy2DExStage", "8-Bit Galaxy" },
    { "MoonWorldShopRoom", "Crazy Cap Store" },
    { "MoonAthleticExStage", "Giant Swings" },
    { "MoonWorldBasementStage", "Moon Cave Escape" },
    { "MoonWorldSphinxRoom", "Moon Kingdom Treasure Vault" },
    { "MoonWorldCaptureParadeStage", "Underground Caverns" },
    { "MoonWorldWeddingRoomStage", "Wedding Room" },
};
KingdomEnglishNameSub subNamesMushroom[] = {
    { "DotHardExStage", "8-Bit Bullet Bills" },
    { "PeachWorldCostumeStage", "Castle Courtyard 64" },
    { "RevengeBossMagmaStage", "Cookatiel Boss Re-fight" },
    { "PeachWorldShopStage", "Crazy Cap Store" },
    { "RevengeBossKnuckleStage", "Knucklotec Boss Re-fight" },
    { "RevengeBossRaidStage", "Lord of Lightning Boss Re-fight" },
    { "RevengeMofumofuStage", "Mechawiggler Boss Re-fight" },
    { "RevengeGiantWanderBossStage", "Mollusque-Lanceur Boss Re-fight" },
    { "PeachWorldPictureBossMagmaStage", "Painting Room: Cookatiel" },
    { "PeachWorldPictureBossKnuckleStage", "Painting Room: Knucklotec" },
    { "PeachWorldPictureBossRaidStage", "Painting Room: Lord of Lightning" },
    { "PeachWorldPictureMofumofuStage", "Painting Room: Mechawiggler" },
    { "PeachWorldPictureGiantWanderBossStage", "Painting Room: Mollusque-Lanceur" },
    { "PeachWorldPictureBossForestStage", "Painting Room: Torkdrift" },
    { "RevengeForestBossStage", "Torkdrift Boss Re-fight" },
    { "PeachWorldCastleStage", "Peach's Castle" },
    { "FukuwaraiMarioStage", "Picture Match" },
    { "YoshiCloudExStage", "Yoshi in the Sea of Clouds" },
};
KingdomEnglishNameSub subNamesDark[] = {
    { "KillerRoadNoCapExStage", "Breakdown Road Capless" },
    { "Special1WorldTowerBombTailStage", "Rabbit Ridge Tower: Hariet Battle" },
    { "Special1WorldTowerStackerStage", "Rabbit Ridge Tower: Topper Battle" },
    { "Special1WorldTowerCapThrowerStage", "Rabbit Ridge Tower: Rango Battle" },
    { "Special1WorldTowerFireBlowerStage", "Rabbit Ridge Tower: Spewart Battle" },
    { "PackunPoisonNoCapExStage", "Invisible Road Capless" },
    { "BikeSteelNoCapExStage", "Vanishing Road Capless" },
    { "SenobiTowerYoshiExStage", "Yoshi on the Sinking Island" },
    { "ShootingCityYoshiExStage", "Yoshi Under Siege" },
    { "LavaWorldUpDownYoshiExStage", "Yoshi's Magma Swamp" },
};
KingdomEnglishNameSub subNamesDarker[] = {
    { "Special2WorldLavaStage", "Inside the Level" },
    { "Special2WorldKoopaStage", "Culmina Crater: Bowser Escape" },
    { "Special2WorldCloudStage", "Culmina Crater: Pokio Section" },
    { "Special2WorldLavaStage", "Inside Culmina Crater" },
};

KingdomEnglishNameMain mainNames[] = {
    { "CapWorldHomeStage", "Cap Kingdom", 2, 3, 4, subNamesCap, 5 },
    { "WaterfallWorldHomeStage", "Cascade Kingdom", 7, 3, 4, subNamesCascade, 5 },
    { "SandWorldHomeStage", "Sand Kingdom", 3, 4, 5, subNamesSand, 16 },
    { "ForestWorldHomeStage", "Wooded Kingdom", 3, 4, 5, subNamesWooded, 14 },
    { "LakeWorldHomeStage", "Lake Kingdom", 2, 3, 4, subNamesLake, 5 },
    { "CloudWorldHomeStage", "Cloud Kingdom", 2, 3, 4, subNamesCloud, 2 },
    { "ClashWorldHomeStage", "Lost Kingdom", 2, 3, 4, subNamesLost, 3 },
    { "CityWorldHomeStage", "Metro Kingdom", 4, 5, 8, subNamesMetro, 17 },
    { "SnowWorldHomeStage", "Snow Kingdom", 2, 3, 4, subNamesSnow, 11 },
    { "SeaWorldHomeStage", "Seaside Kingdom", 2, 3, 4, subNamesSeaside, 10 },
    { "LavaWorldHomeStage", "Luncheon Kingdom", 3, 7, 8, subNamesLuncheon, 12 },
    { "BossRaidWorldHomeStage", "Ruined Kingdom", 2, 3, 4, subNamesRuined, 3 },
    { "SkyWorldHomeStage", "Bowser's Kingdom", 2, 3, 4, subNamesBowser, 8 },
    { "MoonWorldHomeStage", "Moon Kingdom", 2, -1, 3, subNamesMoon, 7 },
    { "PeachWorldHomeStage", "Mushroom Kingdom", -1, 2, 9, subNamesMushroom, 18 },
    { "Special1WorldHomeStage", "Dark Side", 2, -1, 9, subNamesDark, 10 },
    { "Special2WorldHomeStage", "Darker Side", 2, -1, 9, subNamesDarker, 4 },
};

const char* getEnglishName(const char* internalName) {
    for (const auto& entry : mainNames) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesCap) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesCascade) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesSand) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesWooded) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesLake) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesCloud) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesLost) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesMetro) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesSnow) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesSeaside) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesLuncheon) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesRuined) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesBowser) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesMoon) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesMushroom) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesDark) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    for (const auto& entry : subNamesDarker) {
        if (al::isEqualString(entry.mInternal, internalName)) return entry.mEnglish;
    }
    return internalName;
}

inline const char* getScenarioType(KingdomEnglishNameMain& entry, int scenario) {
    if (scenario == 0) return " (NC)";
    if (scenario == 1) return " ()";
    if (scenario == entry.clearMainScenario) return " (Peace)";
    if (scenario == entry.endingScenario) return " (PG)";
    if (scenario == entry.moonRockScenario) return " (MR)";

    return "";
}

void drawStageWarpWindow() {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();

    if (ImGui::CollapsingHeader("Stage Warp")) {
        ImGui::Indent();
        auto curScene = helpers::tryGetStageScene(gameSeq);

        bool isInGame = curScene && curScene->mIsAlive;

        ImGuiContext* ctx = ImGui::GetCurrentContext();

        ImGui::PushItemWidth(200);
        ImGui::InputInt("Scenario", &curScenario);
        if (curScenario < 0) curScenario = 15;
        if (curScenario > 15) curScenario = 0;
        ImGui::PopItemWidth();
        if (ctx->NavId == ImGui::GetID("Scenario"))
            ImGui::SetTooltip("(NC) = No Change, () = First Arrival,\n(PG) = Post-Game, (MR) = Moon Rock");

        for (KingdomEnglishNameMain& entry : mainNames) {
            char popupStr[0x60] = {};
            snprintf(popupStr, sizeof(popupStr), "SubAreaList_%s", entry.mInternal);
            char warpButtonId[0x60] = {};
            sprintf(warpButtonId, "Warp##%s", entry.mInternal);
            char subAreaButtonId[0x60] = {};
            sprintf(subAreaButtonId, "Sub-Areas##%s", entry.mInternal);

            ImGui::AlignTextToFramePadding();
            ImGui::BulletText("%s%s", getEnglishName(entry.mInternal), getScenarioType(entry, curScenario));
            ImGui::SameLine();
            if (ImGui::Button(warpButtonId)) {
                if (!isInGame) continue;
                if (curScenario == 0) curScenario = -1;
                ChangeStageInfo stageInfo(
                    gameSeq->mGameDataHolderAccessor.mData, "start", entry.mInternal, false, curScenario,
                    ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
                );
                gameSeq->mGameDataHolderAccessor.mData->changeNextStage(&stageInfo, 0);
                if (curScenario == -1) curScenario = 0;
            }

            ImGui::SameLine();
            if (ImGui::Button(subAreaButtonId)) ImGui::OpenPopup(popupStr);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Warp to Sub-Area");
            if (ImGui::BeginPopup(popupStr)) {
                for (int i = 0; i < entry.mSubNamesCount - 1; i++) {
                    const auto& subEntry = entry.mSubNames[i];
                    const char* stageName = subEntry.mInternal;

                    if (al::isEqualString(stageName, entry.mInternal)) continue;

                    if (isInGame) {
                        if (ImGui::MenuItem(getEnglishName(stageName))) {
                            if (curScenario == 0) curScenario = -1;
                            ChangeStageInfo stageInfo(
                                gameSeq->mGameDataHolderAccessor.mData, "start", stageName, false,
                                curScenario, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
                            );
                            gameSeq->mGameDataHolderAccessor.mData->changeNextStage(&stageInfo, 0);
                            if (curScenario == -1) curScenario = 0;
                        }
                    } else {
                        ImGui::Text("%s", getEnglishName(stageName));
                    }
                }

                ImGui::EndPopup();
            }
        }
        ImGui::Unindent();
    }
}

// Kingdom (index in mainNames) a home stage or a sub area belongs to, -1 if unknown
static int getKingdomOfStage(const char* stageName) {
    if (!stageName) return -1;
    constexpr int cKingdomNum = sizeof(mainNames) / sizeof(mainNames[0]);
    for (int i = 0; i < cKingdomNum; i++)
        if (al::isEqualString(mainNames[i].mInternal, stageName)) return i;
    for (int i = 0; i < cKingdomNum && i < int(sizeof(btt::sKingdomSubAreas) / sizeof(btt::sKingdomSubAreas[0])); i++)
        for (int j = 0; j < btt::sKingdomSubAreas[i].count; j++)
            if (al::isEqualString(btt::sKingdomSubAreas[i].areas[j].stage, stageName)) return i;
    return -1;
}

// Per mainNames entry, from SystemData/WorldList.szs: MoonRockScenario, or AfterEndingScenario for the kingdoms
// without Moon Rock (Mushroom Kingdom, Dark Side, Darker Side)
static constexpr int cLastScenario[] = { 4, 4, 5, 5, 4, 4, 4, 8, 4, 4, 8, 4, 4, 3, 2, 2, 2 };

int getLastScenarioOfStage(const char* stageName) {
    int kingdom = getKingdomOfStage(stageName);
    return kingdom >= 0 && kingdom < int(sizeof(cLastScenario) / sizeof(cLastScenario[0])) ? cLastScenario[kingdom] : 0;
}

static void changeStage(HakoniwaSequence* gameSeq, const char* label, const char* stageName, int scenario) {
    ChangeStageInfo stageInfo(
        gameSeq->mGameDataHolderAccessor.mData, label, stageName, false, scenario,
        ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
    );
    gameSeq->mGameDataHolderAccessor.mData->changeNextStage(&stageInfo, 0);
}

// Scenario the save is at in a kingdom (scenario -1 would keep the current kingdom's one, which is wrong elsewhere)
static int getSaveScenarioOf(HakoniwaSequence* gameSeq, int kingdomIdx) {
    StageScene* scene = helpers::tryGetStageScene(gameSeq);
    if (!scene || kingdomIdx < 0) return -1;
#if BTT_V130
    // 1.3 inlines tryFindWorldIdByMainStageName: the world index follows mainNames, except Seaside (8) and Snow (9)
    if (kingdomIdx >= int(sizeof(mainNames) / sizeof(mainNames[0]))) return -1;
    s32 worldId = kingdomIdx == 8 ? 9 : kingdomIdx == 9 ? 8 : kingdomIdx;
#else
    s32 worldId = GameDataFunction::tryFindWorldIdByMainStageName((const al::Scene*)scene, mainNames[kingdomIdx].mInternal);
#endif
    if (worldId < 0) return -1;
    s32 scenario = GameDataFunction::getWorldScenarioNo(gameSeq->mGameDataHolderAccessor, worldId);
    return scenario > 0 ? scenario : -1;
}

static char sPendingStage[64] = {};
static char sPendingLabel[64] = {};
static int sPendingGeneration = 0;
static int sPendingFrames = 0;

static const void* sLastScene = nullptr;
static bool sIsLastSceneAlive = false;
static int sSceneGeneration = 0;

int getStageSceneGeneration() {
    return sSceneGeneration;
}

void warpToStageSafe(const char* stageName, const char* label, int scenarioNo) {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
    if (!gameSeq || !gameSeq->mGameDataHolderAccessor.mData || !stageName) return;
    int sc = scenarioNo == 0 ? -1 : scenarioNo;
    int target = getKingdomOfStage(stageName);
    int current = getKingdomOfStage(GameDataFunction::getCurrentStageName(gameSeq->mGameDataHolderAccessor));
    bool isHome = target >= 0 && al::isEqualString(mainNames[target].mInternal, stageName);
    if (target >= 0 && !isHome && target != current) {
        // Load the kingdom first, the sub area is entered once its home stage is up (updateStageWarp)
        snprintf(sPendingStage, sizeof(sPendingStage), "%s", stageName);
        snprintf(sPendingLabel, sizeof(sPendingLabel), "%s", label ? label : "start");
        sPendingGeneration = sSceneGeneration;
        sPendingFrames = 0;
        if (sc == -1) sc = getSaveScenarioOf(gameSeq, target);
        changeStage(gameSeq, "start", mainNames[target].mInternal, sc);
        return;
    }
    sPendingStage[0] = '\0';
    // A kingdom warped to from elsewhere opens at the save's scenario
    if (sc == -1 && isHome && target != current) sc = getSaveScenarioOf(gameSeq, target);
    // A sub area of the kingdom Mario is in keeps the kingdom's scenario (forcing one there could send the game to
    // another stage, e.g. the Secret Flower Field)
    if (!isHome && target >= 0 && target == current) sc = -1;
    changeStage(gameSeq, label ? label : "start", stageName, sc);
}

void updateStageWarp() {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
    StageScene* scene = helpers::tryGetStageScene(gameSeq);
    bool isAlive = scene && scene->mIsAlive;
    if (isAlive && (!sIsLastSceneAlive || scene != sLastScene)) sSceneGeneration++;
    sLastScene = scene;
    sIsLastSceneAlive = isAlive;

    if (!sPendingStage[0]) return;
    if (!gameSeq || !isAlive || sSceneGeneration == sPendingGeneration) {
        sPendingFrames = 0;
        return;
    }
    // Give the kingdom half a second before leaving it again
    if (++sPendingFrames < 30) return;
    changeStage(gameSeq, sPendingLabel, sPendingStage, -1);
    sPendingStage[0] = '\0';
}

bool isStageWarpPending() {
    return sPendingStage[0] != '\0';
}

void warpToStage(const char* stageName, int scenarioNo) {
    warpToStageSafe(stageName, "start", scenarioNo);
}

void warpToKingdom(int kingdomIdx, int scenarioNo) {
    HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
    if (!gameSeq || !gameSeq->mGameDataHolderAccessor.mData) return;
    if (kingdomIdx < 0 || kingdomIdx >= (int)(sizeof(mainNames) / sizeof(mainNames[0]))) return;
    int sc = scenarioNo == 0 ? getSaveScenarioOf(gameSeq, kingdomIdx) : scenarioNo;
    ChangeStageInfo stageInfo(
        gameSeq->mGameDataHolderAccessor.mData, "start", mainNames[kingdomIdx].mInternal, false, sc,
        ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
    );
    gameSeq->mGameDataHolderAccessor.mData->changeNextStage(&stageInfo, 0);
}
