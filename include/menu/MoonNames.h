#pragma once

namespace btt {

// Custom Power Moon names: every lookup of a moon's name ("ScenarioName_<objId>" of its stage message) returns the
// name written by the player instead. Saved in sd:/BTT-Studio/MoonNames.txt (stage <TAB> objId <TAB> name, UTF-8).
void loadMoonNames();
void installMoonNameHooks();

// The moon collected last (from GameDataFunction::setGotShine)
void onMoonGot(const char* stageName, const char* objId);
bool hasLastMoon();
// Name shown for the last moon: the custom one, else the game's one once it has been displayed, else ""
const char* getLastMoonName();
void renameLastMoon(const char* name);
void resetLastMoonName();
void resetAllMoonNames();

// Any moon, by its stage and placement id (see MoonList.h)
const char* getCustomMoonName(const char* stageName, const char* objId); // nullptr if not renamed
void setMoonName(const char* stageName, const char* objId, const char* name); // "" resets it

// One name for every moon ("" = the game's names). Saved in MoonNames.txt as the line "*<TAB>*<TAB>name".
const char* getAllMoonsName();
void setAllMoonsName(const char* name);

} // namespace btt
