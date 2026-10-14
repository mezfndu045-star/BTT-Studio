#pragma once

void drawStageWarpWindow();
const char* getEnglishName(const char* internalName);
void warpToKingdom(int kingdomIdx, int scenarioNo);
void warpToStage(const char* stageName, int scenarioNo);
// Warps to a stage through the given entrance label. A sub area of another kingdom is reached through that kingdom's
// home stage first (warping straight there crashes: the kingdom's resources are not loaded).
void warpToStageSafe(const char* stageName, const char* label, int scenarioNo);
// Second step of warpToStageSafe, called every frame
void updateStageWarp();
bool isStageWarpPending();
// Goes up each time a stage finishes loading (a reloaded StageScene can get the same address as the previous one)
int getStageSceneGeneration();
// Last scenario of the kingdom a stage belongs to (Moon Rocks broken, everything unlocked), 0 if unknown
int getLastScenarioOfStage(const char* stageName);
