// Test channel for the developer: commands written to sd:/BTT-Studio/dbg_cmd.txt (one per line), checked a few times
// per second, results in the emulator log ("DBG: ..."). Used to test warps and refresh options without the menu.
//   kingdom <index> <scenario>          Stages > Warp to Stage
//   stage <stage> <entrance> <scenario> warp to a stage
//   opt <npc|kingdom|warptext|moon|input|input2p> <0|1>, opt style <index>, opt size <percent>, opt body|pressed <color>,
//   opt pattern <index>
//   reload                              end the scene (restart)
//   trace <0|1>                         log the StageScene nerves (start demos)
#include "menu/DebugCommands.h"

#include "nn/fs/fs_files.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/GameDataFunction.h"

#include "helpers/fsHelper.h"
#include "helpers/getHelper.h"
#include "logger.h"
#include "menu/stage_warp.h"
#include "menu/SubAreaImages.h"
#include "imgui.h"
#include "src/settings/SettingsMgr.h"

namespace btt {

namespace {

    constexpr const char* cCmdPath = "sd:/BTT-Studio/dbg_cmd.txt";
    int sPoll = 0;
    bool sIsTrace = false;
    bool sIsCloudWin = false;
    char sDebugImage[96] = {};

    void run(char* line, HakoniwaSequence* gameSeq) {
        char cmd[32] = {}, a[96] = {}, b[96] = {};
        int n = 0;
        int count = sscanf(line, "%31s %95s %95s %d", cmd, a, b, &n);
        if (count < 1) return;
        SettingsMgr::Settings* s = SettingsMgr::getSettings();
        if (strcmp(cmd, "kingdom") == 0) {
            int idx = atoi(a), sc = atoi(b);
            dbgLog("DBG: kingdom %d scenario %d", idx, sc);
            warpToKingdom(idx, sc);
        } else if (strcmp(cmd, "stage") == 0 && count >= 3) {
            dbgLog("DBG: stage %s %s %d", a, b, n);
            warpToStageSafe(a, b, n);
        } else if (strcmp(cmd, "opt") == 0 && s) {
            bool v = atoi(b) != 0;
            if (strcmp(a, "npc") == 0) s->mIsEnableRefreshNpc = v;
            else if (strcmp(a, "kingdom") == 0) s->mIsEnableRefreshKingdomEnter = v;
            else if (strcmp(a, "warptext") == 0) s->mIsEnableRefreshWarpText = v;
            else if (strcmp(a, "moon") == 0) s->mIsEnableMoonRefresh = v;
            else if (strcmp(a, "input") == 0) s->mIsEnableInputDisplay = v;
            else if (strcmp(a, "input2p") == 0) s->mIsEnableInput2P = v;
            else if (strcmp(a, "style") == 0) s->mInputDisplayStyle = u8(atoi(b));
            else if (strcmp(a, "size") == 0) s->mInputDisplaySize = atoi(b);
            else if (strcmp(a, "body") == 0) s->mSkinBodyColor = SettingsMgr::InputDisplayColor(atoi(b));
            else if (strcmp(a, "pattern") == 0) s->mSkinPattern = u8(atoi(b));
            else if (strcmp(a, "pressed") == 0) s->mSkinPressedColor = SettingsMgr::InputDisplayColor(atoi(b));
            dbgLog("DBG: opt %s = %d", a, v);
        } else if (strcmp(cmd, "reload") == 0) {
            if (StageScene* scene = helpers::tryGetStageScene(gameSeq)) scene->kill();
            dbgLog("DBG: reload");
        } else if (strcmp(cmd, "img") == 0) {
            snprintf(sDebugImage, sizeof(sDebugImage), "%s", count >= 2 && strcmp(a, "off") != 0 ? a : "");
            dbgLog("DBG: img %s", sDebugImage);
        } else if (strcmp(cmd, "cloudwin") == 0) {
            sIsCloudWin = true;
            dbgLog("DBG: cloudwin requested");
        } else if (strcmp(cmd, "trace") == 0) {
            sIsTrace = atoi(a) != 0;
            dbgLog("DBG: trace %d", sIsTrace);
        }
    }

} // namespace

void drawDebugImage() {
    if (!sDebugImage[0]) return;
    if (ImTextureID tex = getRomfsImage(sDebugImage))
        ImGui::GetForegroundDrawList()->AddImage(tex, ImVec2(600, 250), ImVec2(600 + 384 * 1.5f, 250 + 216 * 1.5f));
}

bool takeDbgCloudWin() {
    bool r = sIsCloudWin;
    sIsCloudWin = false;
    return r;
}

bool isDebugTrace() {
    return sIsTrace;
}

// Trace lines kept in memory and written to sd:/BTT-Studio/dbg_log.txt (the emulator log is written late)
static char sLog[24 * 1024];
static int sLogLen = 0;
void dbgLog(const char* fmt, ...) {
    char line[512];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line) - 2, fmt, args);
    va_end(args);
    if (n < 0) return;
    if (n > (int)sizeof(line) - 2) n = sizeof(line) - 2;
    line[n++] = 10;
    if (sLogLen + n >= (int)sizeof(sLog)) sLogLen = 0;
    memcpy(sLog + sLogLen, line, n);
    sLogLen += n;
    FsHelper::writeFileToPath(sLog, sLogLen, "sd:/BTT-Studio/dbg_log.txt");
}

void updateDebugCommands() {
    // Developer builds only (cmake -DBTT_DEBUG_COMMANDS=1): released builds do not poll the SD card
#if !BTT_DEBUG_COMMANDS
    return;
#endif
    if (++sPoll < 15) return;
    sPoll = 0;
    if (!FsHelper::isFileExist(cCmdPath)) return;
    FsHelper::LoadData data = { .path = cCmdPath };
    FsHelper::loadFileFromPath(data, nullptr);
    nn::fs::DeleteFile(cCmdPath);
    if (!data.buffer) return;
    char* text = static_cast<char*>(malloc(data.bufSize + 1));
    if (text) {
        memcpy(text, data.buffer, data.bufSize);
        text[data.bufSize] = '\0';
        HakoniwaSequence* gameSeq = helpers::tryGetHakoniwaSequence();
        for (char* line = text; line && *line;) {
            char* next = strpbrk(line, "\r\n");
            if (next) *next++ = '\0';
            if (*line) run(line, gameSeq);
            line = next;
        }
        free(text);
    }
    free(data.buffer);
}

} // namespace btt
