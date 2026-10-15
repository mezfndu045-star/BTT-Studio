#pragma once

namespace btt {

class Menu;

// Quick wheel: 6, 8 or 12 slots (Settings::mWheelSize) holding any on/off option or button of the Options, Misc and
// Timer categories, or one of the extra actions (Warp Last CP). Slots are saved by name (Settings::mWheelSlots).
// Mario keeps moving while it is open: only the right stick (points at a slot) goes to the wheel. Releasing the held
// key (L / R) or the stick (L3 / R3) runs the slot pointed at.
// - L3 / R3: click to open, click again to run the slot pointed at (or close without one)
// - L / R (hold): hold to open, release to run the slot pointed at; another button pressed meanwhile cancels it
//   (L / R + D-Pad hotkeys keep working)
constexpr int cWheelSlotNum = 12;

// Wheel key choices (Settings::mWheelKey, only append)
constexpr const char* cWheelKeyNames[] = { "Off", "L3", "R3", "L3 or R3", "L (hold)", "R (hold)" };
// Wheel sizes (Settings::mWheelSize)
constexpr const char* cWheelSizeNames[] = { "6", "8", "12" };
constexpr int cWheelSizes[] = { 6, 8, 12 };

// Makes sure the menu (and so the action list) is built: render thread, every frame before drawing
void prepareQuickWheel(Menu* menu);
// Builds the action list from the Options, Misc and Timer categories (Menu::initCategories, before Keybinds)
void buildWheelActions(Menu* menu);
// Names of the actions, index 0 is "Empty"; valid once prepareQuickWheel ran
int getWheelActionNum();
const char* const* getWheelActionNames();
int findWheelAction(const char* name);

// Game thread, every frame (before the hotkeys)
void updateQuickWheel(Menu* menu);
void drawQuickWheel();
bool isQuickWheelOpen();
// Hold modes let the hotkeys run (L / R + D-Pad)
bool isQuickWheelBlockingHotkeys();

} // namespace btt
