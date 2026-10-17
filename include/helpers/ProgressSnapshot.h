#pragma once

class GameDataHolder;

namespace btt {

// Moon Refresh: the progress (moons, story, coins...) as it was when the current scene started, written by the
// game's own save code (GameDataFile::writeToStream) and read back by Restart Scene (initializeData +
// tryReadByamlData), so moons and story steps of a try are undone.

// StageScene::init: remember the progress (only with Moon Refresh)
void takeProgressSnapshot(GameDataHolder* holder);
// Restart Scene: put the progress back and restart the current stage at the same entrance. False when there is no
// snapshot (Moon Refresh off): the caller restarts the scene the usual way.
bool restoreProgressSnapshot(GameDataHolder* holder);
// The entrance the stage restarts at (GameDataHolder::getNextPlayerStartId; null or empty: none, the stage start)
const char* ProgressSnapshot_getRestartStartId(GameDataHolder* holder);
// Odyssey travel to a kingdom at scenario 1 (Moon Refresh / Gray Moon Refresh): the first travel to that kingdom keeps
// the progress, the next ones to the same kingdom put it back (moons and story steps since then undone). Called
// before the world warp is prepared. True when the progress was put back.
bool takeOrRestoreTravelSnapshot(GameDataHolder* holder, const char* stageName);

} // namespace btt
