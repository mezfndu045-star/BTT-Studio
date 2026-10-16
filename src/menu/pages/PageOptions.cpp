#include "imgui.h"
#include "imgui_internal.h"
#include "menu/Menu.h"

using namespace btt;

void Menu::drawPageOptions() {
    if (ImGui::CollapsingHeader("Options")) {
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Refresh")) {
            ImGui::Indent();
            ImGui::Checkbox("Disable Saving Moons", &set->getSettings()->mIsEnableMoonRefresh);
            ImGui::Checkbox("Refresh Gray Moons", &set->getSettings()->mIsEnableGrayMoonRefresh);
            ImGui::Checkbox(
                "Always Manually Skip Cutscene", &set->getSettings()->mIsEnableAlwaysManualCutscene
            );
            ImGui::Checkbox("Refresh Purple Coins", &set->getSettings()->mIsEnableRefreshPurps);
            ImGui::Checkbox("Refresh Doors", &set->getSettings()->mIsEnableDoorRefresh);
            ImGui::Checkbox("Refresh Seeds", &set->getSettings()->mIsEnableFlowerPotRefresh);
            ImGui::Checkbox("Seeds Growing", &set->getSettings()->mIsEnableSeedGrowing);
            ImGui::Checkbox("Refresh Toad Text", &set->getSettings()->mIsEnableRefreshNpc);
            ImGui::Checkbox("Text Refresh", &set->getSettings()->mIsEnableRefreshWarpText);
            ImGui::Checkbox("Disable Saving Coin Stacks", &set->getSettings()->mIsEnableDisableCoinStackSave);
            ImGui::Checkbox(
                "Refresh Kingdom Enter Cutscenes", &set->getSettings()->mIsEnableRefreshKingdomEnter
            );
            if (GImGui->NavId == ImGui::GetID("Refresh Kingdom Enter Cutscenes")) {
                ImGui::SetTooltip(
                    "Doesn't refresh Cappy text after landing in Cascade, Sand, Metro, Lost and Moon."
                );
            }

            ImGui::Checkbox("Refresh Moon Shards", &set->getSettings()->mIsEnableShardRefresh);

            ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Disable")) {
            ImGui::Indent();
            ImGui::Checkbox("Disable Auto Save", &set->getSettings()->mIsEnableDisableAutoSave);
            ImGui::Checkbox("Disable Moon Lock", &set->getSettings()->mIsEnableDisableMoonLock);
            ImGui::Checkbox("No Damage", &set->getSettings()->mIsEnableNoDamage);
            ImGui::Checkbox("Disable Music", &set->getSettings()->mIsEnableDisableMusic);
            ImGui::Checkbox("Disable Teleport Puppet", &set->getSettings()->mIsEnableDisableTpPuppet);
            if (GImGui->NavId == ImGui::GetID("Disable Teleport Puppet"))
                ImGui::SetTooltip("Only in Moon Get Animation");
            ImGui::Checkbox("No Checkpoint Touch", &set->getSettings()->mIsEnableNoCheckpointTouch);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Misc##options")) {
            ImGui::Indent();
            ImGui::Checkbox("Always Allow Checkpoints", &set->getSettings()->mIsEnableAlwaysCheckpoints);
            ImGui::Checkbox("Skip Cloud", &set->getSettings()->mIsEnableSkipCloud);
            ImGui::Checkbox("Skip Broodals", &set->getSettings()->mIsEnableSkipBroodals);
            if (GImGui->NavId == ImGui::GetID("Skip Broodals"))
                ImGui::SetTooltip("Ends Broodal fights right after their intro and spawns the Multi Moon");
            ImGui::Checkbox("Always Odyssey Power Up", &set->getSettings()->mIsEnableAlwaysOdysseyPowerUp);
            if (GImGui->NavId == ImGui::GetID("Always Odyssey Power Up"))
                ImGui::SetTooltip("Paying any number of moons to the Odyssey plays the power up cutscene");
            ImGui::Checkbox("Odyssey In All Kingdoms", &set->getSettings()->mIsEnableOdysseyInAllKingdoms);
            if (GImGui->NavId == ImGui::GetID("Odyssey In All Kingdoms"))
                ImGui::SetTooltip("The Odyssey is there even in kingdoms the save has not reached yet");
            ImGui::Checkbox("Night Metro Multi Moon", &set->getSettings()->mIsEnableNightMetroMultiMoon);
            if (GImGui->NavId == ImGui::GetID("Night Metro Multi Moon"))
                ImGui::SetTooltip("Mechawiggler's Multi Moon appears above Mario at the start of Night Metro");
            ImGui::Checkbox("Enable All Checkpoints", &set->getSettings()->mIsEnableAllCheckpoints);
            ImGui::Checkbox("Luigi Pixels", &set->getSettings()->mIsEnableLuigiPixels);
            ImGui::Checkbox("Spawn all Hint Art Moons", &set->getSettings()->mIsEnableHintPhotoSpawn);

            ImGui::Unindent();
        }

        ImGui::Unindent();
    }
}
