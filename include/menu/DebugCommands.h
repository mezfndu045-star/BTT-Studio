#pragma once

namespace btt {

// Developer test channel (sd:/BTT-Studio/dbg_cmd.txt, see DebugCommands.cpp), game thread every frame
void updateDebugCommands();
// "trace 1": log the StageScene nerves and the stage start demos
bool isDebugTrace();
// Test channel "press": the button the mod holds this frame (-1: none)
int getDebugPressedButton();
// Trace line for sd:/BTT-Studio/dbg_log.txt
void dbgLog(const char* fmt, ...);
// "cloudwin": true once (KoopaLv1::movement then defeats Bowser)
bool takeDbgCloudWin();
// "img <romfs path>" / "img off": draws that picture (test of the romfs picture loading)
void drawDebugImage();

} // namespace btt
