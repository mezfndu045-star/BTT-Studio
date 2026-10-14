#include "menu/MoonNames.h"

#include "hk/hook/Trampoline.h"

#include "nn/fs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutActorUtil.h"

#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"

#include "helpers/fsHelper.h"
#include "logger.h"

namespace al {
class IUseMessageSystem;
}

namespace btt {

namespace {

    constexpr const char* cPath = "sd:/BTT-Studio/MoonNames.txt";
    constexpr int cMaxNames = 256;
    constexpr int cMaxChars = 64;

    struct MoonName {
        char stage[64];
        char objId[32];
        char utf8[cMaxChars * 3 + 1];
        char16_t utf16[cMaxChars + 1];
    };

    MoonName sNames[cMaxNames];
    int sNameNum = 0;

    // Name given to every moon at once
    char sAllUtf8[cMaxChars * 3 + 1] = {};
    char16_t sAllUtf16[cMaxChars + 1] = {};

    char sLastStage[64] = {};
    char sLastObjId[32] = {};
    char sLastOriginal[cMaxChars * 3 + 1] = {};

    void toUtf16(const char* in, char16_t* out, int max) {
        int n = 0;
        const unsigned char* s = (const unsigned char*)in;
        while (*s && n < max) {
            unsigned c = *s++;
            if (c >= 0xE0 && s[0] && s[1]) {
                c = ((c & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F);
                s += 2;
            } else if (c >= 0xC0 && s[0]) {
                c = ((c & 0x1F) << 6) | (s[0] & 0x3F);
                s += 1;
            }
            out[n++] = char16_t(c);
        }
        out[n] = 0;
    }

    void toUtf8(const char16_t* in, char* out, int maxBytes) {
        int n = 0;
        for (; *in && n < maxBytes - 3; in++) {
            unsigned c = *in;
            // game strings may hold control tags (0x0E ...): stop there
            if (c == 0x0E || c < 0x20) break;
            if (c < 0x80) out[n++] = char(c);
            else if (c < 0x800) {
                out[n++] = char(0xC0 | (c >> 6));
                out[n++] = char(0x80 | (c & 0x3F));
            } else {
                out[n++] = char(0xE0 | (c >> 12));
                out[n++] = char(0x80 | ((c >> 6) & 0x3F));
                out[n++] = char(0x80 | (c & 0x3F));
            }
        }
        out[n] = 0;
    }

    MoonName* find(const char* stage, const char* objId) {
        for (int i = 0; i < sNameNum; i++)
            if (strcmp(sNames[i].stage, stage) == 0 && strcmp(sNames[i].objId, objId) == 0) return &sNames[i];
        return nullptr;
    }

    void remove(MoonName* entry) {
        int i = int(entry - sNames);
        memmove(&sNames[i], &sNames[i + 1], sizeof(MoonName) * (sNameNum - i - 1));
        sNameNum--;
    }

    void save() {
        static char buf[cMaxNames * (64 + 32 + cMaxChars * 3 + 4)];
        size_t len = 0;
        if (sAllUtf8[0]) len += snprintf(buf + len, sizeof(buf) - len, "*\t*\t%s\n", sAllUtf8);
        for (int i = 0; i < sNameNum; i++)
            len += snprintf(buf + len, sizeof(buf) - len, "%s\t%s\t%s\n", sNames[i].stage, sNames[i].objId, sNames[i].utf8);
        if (!FsHelper::isDirExist("sd:/BTT-Studio")) nn::fs::CreateDirectory("sd:/BTT-Studio");
        FsHelper::writeFileToPath(buf, len, cPath);
    }

    void set(const char* stage, const char* objId, const char* name) {
        if (strcmp(stage, "*") == 0) {
            snprintf(sAllUtf8, sizeof(sAllUtf8), "%s", name ? name : "");
            toUtf16(sAllUtf8, sAllUtf16, cMaxChars);
            return;
        }
        MoonName* entry = find(stage, objId);
        if (!name || !name[0]) {
            if (entry) remove(entry);
            return;
        }
        if (!entry) {
            if (sNameNum >= cMaxNames) return;
            entry = &sNames[sNameNum++];
            snprintf(entry->stage, sizeof(entry->stage), "%s", stage);
            snprintf(entry->objId, sizeof(entry->objId), "%s", objId);
        }
        snprintf(entry->utf8, sizeof(entry->utf8), "%s", name);
        toUtf16(entry->utf8, entry->utf16, cMaxChars);
    }

    // "ScenarioName_<objId>" -> custom name of that moon of the stage
    const char16_t* findCustom(const char* stage, const char* label, const char16_t* original) {
        if (!stage || !label || strncmp(label, "ScenarioName_", 13) != 0) return nullptr;
        const char* objId = label + 13;
        if (original && sLastStage[0] && strcmp(stage, sLastStage) == 0 && strcmp(objId, sLastObjId) == 0)
            toUtf8(original, sLastOriginal, sizeof(sLastOriginal));
        MoonName* entry = find(stage, objId);
        if (entry) return entry->utf16;
        return sAllUtf8[0] ? sAllUtf16 : nullptr;
    }

    bool isMoonNameLabel(const char* label) { return label && strncmp(label, "ScenarioName_", 13) == 0; }

} // namespace

HkTrampoline getStageMessageStringHook = [](TrampolineStatic(), const al::IUseMessageSystem* system, const char* stage,
                                            const char* label) -> const char16_t* {
    const char16_t* original = orig(system, stage, label);
    const char16_t* custom = findCustom(stage, label, original);
    return custom ? custom : original;
};

HkTrampoline tryGetStageMessageStringHook = [](TrampolineStatic(), const char16_t** out, const al::IUseMessageSystem* system,
                                               const char* stage, const char* label) -> bool {
    bool found = orig(out, system, stage, label);
    const char16_t* custom = findCustom(stage, label, found && out ? *out : nullptr);
    if (custom && out) {
        *out = custom;
        return true;
    }
    return found;
};

// Moon Get screen (StageSceneStateGetShine...) and other layouts: rs::trySetPaneStageMessageIfExist checks the label
// itself and sets the pane, without the message functions above
HkTrampoline trySetPaneStageMessageIfExistHook = [](TrampolineStatic(), al::LayoutActor* layout, const char* pane,
                                                    const char* label, const char* stage) -> bool {
    // With Moon Refresh the collected moon is not registered, so the Moon Get screen asks for an empty label (and the
    // game falls back on the "Moon Refresh Text", BTT Studio): use the moon that was just collected instead
    char lastLabel[64];
    if ((!label || !label[0]) && sLastStage[0]) {
        snprintf(lastLabel, sizeof(lastLabel), "ScenarioName_%s", sLastObjId);
        label = lastLabel;
        stage = sLastStage;
    }
    const char* stageName = stage && stage[0] ? stage : GameDataFunction::getCurrentStageName(GameDataHolderAccessor(layout));
    const char16_t* custom = findCustom(stageName, label, nullptr);
    if (!custom && sAllUtf8[0] && isMoonNameLabel(label)) custom = sAllUtf16;
    if (custom && layout) {
        al::setPaneString(layout, pane, custom, 0);
        return true;
    }
    return orig(layout, pane, label, stage && stage[0] ? stage : nullptr);
};

void installMoonNameHooks() {
    trySetPaneStageMessageIfExistHook.installAtSym<"_ZN2rs29trySetPaneStageMessageIfExistEPN2al11LayoutActorEPKcS4_S4_">();
    getStageMessageStringHook.installAtSym<"_ZN2al21getStageMessageStringEPKNS_17IUseMessageSystemEPKcS4_">();
    tryGetStageMessageStringHook.installAtSym<"_ZN2al24tryGetStageMessageStringEPPKDsPKNS_17IUseMessageSystemEPKcS7_">();
}

void loadMoonNames() {
    FsHelper::LoadData data = { .path = cPath };
    FsHelper::loadFileFromPath(data, nullptr);
    if (!data.buffer) return;
    char* text = (char*)malloc(data.bufSize + 1);
    if (!text) {
        free(data.buffer);
        return;
    }
    memcpy(text, data.buffer, data.bufSize);
    text[data.bufSize] = '\0';
    free(data.buffer);
    sNameNum = 0;
    for (char* line = text; line && *line;) {
        char* next = strchr(line, '\n');
        if (next) *next++ = '\0';
        char* tab1 = strchr(line, '\t');
        char* tab2 = tab1 ? strchr(tab1 + 1, '\t') : nullptr;
        if (tab2) {
            *tab1 = '\0';
            *tab2 = '\0';
            char* end = tab2 + 1 + strlen(tab2 + 1);
            if (end > tab2 + 1 && end[-1] == '\r') end[-1] = '\0';
            set(line, tab1 + 1, tab2 + 1);
        }
        line = next;
    }
    free(text);
    Logger::instance()->log(Logger::LogType::LogInfo, "MoonNames: %d custom names loaded", sNameNum);
}

void onMoonGot(const char* stageName, const char* objId) {
    if (!stageName || !objId) return;
    if (strcmp(sLastStage, stageName) != 0 || strcmp(sLastObjId, objId) != 0) sLastOriginal[0] = '\0';
    snprintf(sLastStage, sizeof(sLastStage), "%s", stageName);
    snprintf(sLastObjId, sizeof(sLastObjId), "%s", objId);
}

bool hasLastMoon() {
    return sLastStage[0] != '\0';
}

const char* getLastMoonName() {
    if (!hasLastMoon()) return "";
    MoonName* entry = find(sLastStage, sLastObjId);
    return entry ? entry->utf8 : sLastOriginal;
}

void renameLastMoon(const char* name) {
    if (!hasLastMoon()) return;
    set(sLastStage, sLastObjId, name);
    save();
}

void resetLastMoonName() {
    renameLastMoon("");
}

const char* getCustomMoonName(const char* stageName, const char* objId) {
    MoonName* entry = find(stageName, objId);
    return entry ? entry->utf8 : nullptr;
}

void setMoonName(const char* stageName, const char* objId, const char* name) {
    set(stageName, objId, name);
    save();
}

const char* getAllMoonsName() {
    return sAllUtf8;
}

void setAllMoonsName(const char* name) {
    set("*", "*", name);
    save();
}

void resetAllMoonNames() {
    sNameNum = 0;
    sAllUtf8[0] = '\0';
    sAllUtf16[0] = 0;
    save();
}

} // namespace btt
