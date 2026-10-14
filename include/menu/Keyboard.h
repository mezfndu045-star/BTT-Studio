#pragma once

#include <functional>

namespace btt {

// On-screen keyboard used with the controller (works on Switch and yuzu): D-Pad moves, A types the key,
// B erases, Y toggles caps, + validates, - cancels. Shown over the menu while open.
void openKeyboard(const char* title, const char* initialText, std::function<void(const char*)> onValidate);
bool isKeyboardOpen();
void updateKeyboardInput();
void drawKeyboard();

} // namespace btt
