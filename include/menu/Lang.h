#pragma once

namespace btt {

// Menu language (Settings::mLanguage, only append)
constexpr const char* cLanguageNames[] = { "English", "Français" };

bool isFrench();

// Text of an English menu string in the chosen language (the same pointer in English or without a translation).
// Only for strings that live as long as the module (literals, static tables): results are cached by pointer.
const char* tr(const char* en);

// ": " in English, " : " in French (space before a colon)
const char* trColon();

} // namespace btt
