// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Sebastian Merz
// QuickContinue.SA.asi - GTA San Andreas (v1.0 US, 32-bit)
//
//  1. Adds a "Continue" entry at the top of the main menu that loads the newest
//     save game directly (one click instead of Start Game > Load Game > slot > Yes).
//  2. Optionally (AutoLoad=1 in QuickContinue.SA.ini) boots straight into that save without
//     showing the menu at all. Hold the skip key (default Ctrl) to get the menu anyway.
//
// "Continue" is a normal menu item that switches to SCREEN_LOAD_FIRST_SAVE - the same
// screen the game reaches after confirming "Load this game?" - with m_SelectedSlot set to
// the newest slot, so the game's own load path (CMenuManager::ProcessFileActions) does the
// work. AutoLoad starts that same load path without showing the menu.
// See README.md.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "game.h"

using namespace game;

constexpr const char* kVersion = "1.0.0";

// ---------------------------------------------------------------------------
// Log (QuickContinue.SA.log in "Documents\GTA San Andreas User Files")
// ---------------------------------------------------------------------------
static char g_logPath[MAX_PATH];
static bool g_debug = false; // Debug=1 in the ini: verbose log (state changes, patch details)

static void Log(const char* fmt, ...) {
    if (!g_debug || !g_logPath[0]) return;   // no log unless Debug=1
    FILE* f = nullptr;
    if (fopen_s(&f, g_logPath, "a") != 0 || !f) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

#define LOG_DEBUG(...) do { if (g_debug) Log(__VA_ARGS__); } while (0)

// ---------------------------------------------------------------------------
// Settings (QuickContinue.SA.ini next to the .asi)
// ---------------------------------------------------------------------------
static bool g_continueButton = true;
static bool g_autoLoad       = false;
static int  g_skipKey        = VK_CONTROL;
static bool g_skipIntro      = true;
static bool g_skipTitleFade  = true;
static bool g_smoothLoad     = true;
static bool g_pauseLoadLatest = true;   // "Load latest save" entry in the pause menu
static bool g_pauseConfirm   = true;    // ask "all unsaved progress will be lost" before loading from the pause menu
static bool g_settingsMenu   = true;    // "QuickContinue" page in Options; its values are kept in QuickContinue.SA.cfg
static char g_cfgPath[MAX_PATH] = "";
static char g_iniPath[MAX_PATH] = "";
// [Text] in the ini: own wording for the two entries (any language); empty = the built-in text of the game language
static char g_language[8]        = "";   // Language= in the ini: en, de, fr, it, es, pt, nl, ... (empty = game language)
static char g_textContinue[48]   = "";
static char g_textLoadLatest[48] = "";

static void LoadSavedSettings();
static void LoadPageTextOverrides(const char* ini);

static void LoadSettings(HMODULE self) {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(self, path, MAX_PATH)) return;
    char* dot = strrchr(path, '.');
    if (!dot) return;
    // The game folder is often not writable for the game process: log into the user files folder instead.
    char docs[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs))) {
        _snprintf_s(g_logPath, MAX_PATH, _TRUNCATE, "%s\\GTA San Andreas User Files\\QuickContinue.SA.log", docs);
    }
    strcpy_s(dot, path + MAX_PATH - dot, ".ini");
    strcpy_s(g_iniPath, path);
    g_debug          = GetPrivateProfileIntA("QuickContinue", "Debug", 0, path) != 0;
    if (g_debug && g_logPath[0]) remove(g_logPath);   // a fresh log per start
    Log("QuickContinue.SA %s loaded from %s", kVersion, path);
    g_continueButton = GetPrivateProfileIntA("QuickContinue", "ContinueButton", 1, path) != 0;
    g_autoLoad       = GetPrivateProfileIntA("QuickContinue", "AutoLoad", 0, path) != 0;
    g_skipKey        = GetPrivateProfileIntA("QuickContinue", "SkipKey", VK_CONTROL, path);
    g_skipIntro      = GetPrivateProfileIntA("QuickContinue", "SkipIntro", 1, path) != 0;
    g_skipTitleFade  = GetPrivateProfileIntA("QuickContinue", "SkipTitleFade", 1, path) != 0;
    g_smoothLoad     = GetPrivateProfileIntA("QuickContinue", "SmoothLoadScreen", 1, path) != 0;
    g_pauseLoadLatest = GetPrivateProfileIntA("QuickContinue", "PauseMenuLoadLatest", 1, path) != 0;
    g_pauseConfirm   = GetPrivateProfileIntA("QuickContinue", "PauseMenuConfirm", 1, path) != 0;
    g_settingsMenu   = GetPrivateProfileIntA("QuickContinue", "SettingsMenu", 1, path) != 0;
    GetPrivateProfileStringA("QuickContinue", "Language", "", g_language, sizeof(g_language), path);
    GetPrivateProfileStringA("Text", "Continue", "", g_textContinue, sizeof(g_textContinue), path);
    GetPrivateProfileStringA("Text", "LoadLatest", "", g_textLoadLatest, sizeof(g_textLoadLatest), path);
    LoadSavedSettings();
    LoadPageTextOverrides(path);
}

// ---------------------------------------------------------------------------
// Newest save game
// ---------------------------------------------------------------------------
// Returns the 0-based slot of the most recently written GTASAsfN.b, or -1.
static int FindNewestSlot() {
    char docs[MAX_PATH];
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs))) return -1;

    int      best = -1;
    uint64_t bestTime = 0;
    for (int i = 1; i <= MAX_SAVEGAME_SLOTS; i++) {
        char path[MAX_PATH];
        _snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\GTA San Andreas User Files\\GTASAsf%d.b", docs, i);
        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) continue;
        if (fad.nFileSizeHigh == 0 && fad.nFileSizeLow == 0) continue;
        const uint64_t t = (uint64_t(fad.ftLastWriteTime.dwHighDateTime) << 32) | fad.ftLastWriteTime.dwLowDateTime;
        if (best < 0 || t > bestTime) { best = i - 1; bestTime = t; }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Label for the new menu entry (the game's GXT has no "Continue" key)
// ---------------------------------------------------------------------------
static const char KEY_CONTINUE[]  = "QC_CONT";   // main menu: Continue
static const char KEY_LOADLATEST[] = "QC_LOAD";   // pause menu: Load latest save
static char       g_label[48]     = "Continue";

struct Lang { const char* code; const char* cont; const char* load; };

// Texts are UTF-8 here and converted to the game's own character codes (Gxt below). The game font has the letters of
// its five languages (ÀÁÂÄÆÇÈÉÊËÌÍÎÏÒÓÔÖÙÚÛÜßàáâäæçèéêëìíîïòóôöùúûüñÑ¿¡); languages with other letters are written without them.
static const Lang kLangs[] = {
    { "en", "Continue",   "Load latest save" },
    { "de", "Fortsetzen", "Letzten Spielstand laden" },
    { "fr", "Continuer",  "Charger la dernière partie" },
    { "it", "Continua",   "Carica ultimo salvataggio" },
    { "es", "Continuar",  "Cargar última partida" },
    { "pt", "Continuar",  "Carregar último jogo" },
    { "nl", "Doorgaan",   "Laatste spel laden" },
    { "sv", "Fortsätt",   "Ladda senaste sparning" },
    { "fi", "Jatka",      "Lataa viimeisin tallennus" },
    { "hu", "Folytatás",  "Utolsó mentés betöltése" },
};

static const Lang* FindLang(const char* code) {
    for (const Lang& l : kLangs) if (_stricmp(l.code, code) == 0) return &l;
    return nullptr;
}

// The language: the ini key Language (any code of kLangs) or, if empty / unknown, the language slot of the game.
static const Lang& CurrentLang() {
    if (g_language[0]) { if (const Lang* l = FindLang(g_language)) return *l; }
    switch (Menu().m_nPrefsLanguage) {
    case GXT_LANG_FRENCH:  return *FindLang("fr");
    case GXT_LANG_GERMAN:  return *FindLang("de");
    case GXT_LANG_ITALIAN: return *FindLang("it");
    case GXT_LANG_SPANISH: return *FindLang("es");
    default:               return *FindLang("en");
    }
}

// UTF-8 -> the character codes of the game font. Letters the font does not have become their plain ASCII base.
static const char* Gxt(const char* utf8) {
    static const char kFont[] = "ÀÁÂÄÆÇÈÉÊËÌÍÎÏÒÓÔÖÙÚÛÜßàáâäæçèéêëìíîïòóôöùúûüñÑ¿¡";   // codes 0x80...
    static char ring[8][96];
    static int  next = 0;
    char* out = ring[next++ & 7];
    int n = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8); *p && n < 94; ) {
        if (*p < 0x80) { out[n++] = static_cast<char>(*p++); continue; }
        const int len = (*p >= 0xF0) ? 4 : (*p >= 0xE0) ? 3 : 2;
        // look the character up in kFont (itself UTF-8, two bytes per letter)
        int code = -1;
        if (len == 2) {
            for (int i = 0, c = 0x80; kFont[i]; i += 2, c++)
                if (static_cast<unsigned char>(kFont[i]) == p[0] && static_cast<unsigned char>(kFont[i + 1]) == p[1]) { code = c; break; }
        }
        out[n++] = code >= 0 ? static_cast<char>(code) : '?';
        p += len;
    }
    out[n] = 0;
    return out;
}

using TextGetFn = const char*(__thiscall*)(void* self, const char* key);
static TextGetFn g_textGetOrig = nullptr;

static const char* SettingsText(void* self, const char* key);   // texts of the settings page, defined below
static const char* SettingsValueText(void* self);
static int g_rowPending = -1;   // settings page: row whose name was just requested, the next text request is its value

// The value column of the settings rows. Another plugin (a language loader) may have redirected this call, so it
// is hooked on its own, with whatever target it has, and chained to.
static TextGetFn g_valueGetOrig = nullptr;
static const char* __fastcall ValueGetHook(void* self, void* /*edx*/, const char* key) {
    if (g_rowPending >= 0) return SettingsValueText(self);
    return g_valueGetOrig(self, key);
}

static const char* __fastcall TextGetHook(void* self, void* /*edx*/, const char* key) {
    if (key && key[0] == 'Q' && key[1] == 'C' && key[2] == '_') {
        // GxtChar is plain ASCII for these characters
        if (strcmp(key, KEY_CONTINUE) == 0) {
            strcpy_s(g_label, g_textContinue[0] ? g_textContinue : Gxt(CurrentLang().cont));
            return g_label;
        }
        if (strcmp(key, KEY_LOADLATEST) == 0) {
            strcpy_s(g_label, g_textLoadLatest[0] ? g_textLoadLatest : Gxt(CurrentLang().load));
            return g_label;
        }
        if (const char* t = SettingsText(self, key)) return t;
    } else if (g_rowPending >= 0 && key && key[0] == 'F' && key[1] == 'E' && key[2] == 'L' && key[3] == '_') {
        return SettingsValueText(self);
    }
    return g_textGetOrig(self, key);
}

// ---------------------------------------------------------------------------
// Patching helpers
// ---------------------------------------------------------------------------
static void WriteMem(uintptr_t addr, const void* data, size_t len) {
    DWORD old;
    if (VirtualProtect(reinterpret_cast<void*>(addr), len, PAGE_EXECUTE_READWRITE, &old)) {
        memcpy(reinterpret_cast<void*>(addr), data, len);
        VirtualProtect(reinterpret_cast<void*>(addr), len, old, &old);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(addr), len);
    }
}

static void PatchRel(uintptr_t site, int32_t rel) { WriteMem(site + 1, &rel, 4); }

// Returns the call target if `site` is an E8 call to `expectedTarget` (0 = any), else 0.
static uintptr_t CheckCallSite(uintptr_t site, uintptr_t expectedTarget) {
    if (*reinterpret_cast<uint8_t*>(site) != 0xE8) return 0;
    int32_t rel; memcpy(&rel, reinterpret_cast<void*>(site + 1), 4);
    const uintptr_t target = site + 5 + rel;
    return (expectedTarget == 0 || target == expectedTarget) ? target : 0;
}

// Redirects every `call CText::Get` inside the CMenuManager code to TextGetHook.
static int HookMenuTextCalls() {
    int count = 0;
    for (uintptr_t a = ADDR_MenuCodeBegin; a < ADDR_MenuCodeEnd - 5; a++) {
        if (!CheckCallSite(a, ADDR_CText_Get)) continue;
        PatchRel(a, static_cast<int32_t>(reinterpret_cast<uintptr_t>(&TextGetHook) - (a + 5)));
        count++;
    }
    if (count) g_textGetOrig = reinterpret_cast<TextGetFn>(ADDR_CText_Get);
    const uintptr_t valueTarget = CheckCallSite(ADDR_MenuValueTextCall, 0);
    if (valueTarget && valueTarget != reinterpret_cast<uintptr_t>(&TextGetHook)) {
        g_valueGetOrig = reinterpret_cast<TextGetFn>(valueTarget);
        PatchRel(ADDR_MenuValueTextCall, static_cast<int32_t>(reinterpret_cast<uintptr_t>(&ValueGetHook) - (ADDR_MenuValueTextCall + 5)));
        LOG_DEBUG("value text call %08X: target %08X%s", (unsigned)ADDR_MenuValueTextCall, (unsigned)valueTarget,
                  valueTarget == ADDR_CText_Get ? "" : " (redirected by another plugin)");
    }
    return count;
}

// ---------------------------------------------------------------------------
// Main menu: insert "Continue" as the first entry
// ---------------------------------------------------------------------------
static bool g_menuPatched = false;
static int8_t g_startGameTarget, g_optionsTarget, g_quitTarget;

static bool InsertContinueEntry(MenuScreen& s) {
    MenuItem* it = s.m_aItems;

    // Expect exactly: Start Game, Options, Quit Game, <empty>.
    if (strcmp(s.m_szTitleName, "FEM_MM") != 0 || strcmp(it[0].m_szName, "FEP_STG") != 0 ||
        strcmp(it[1].m_szName, "FEP_OPT") != 0 || strcmp(it[2].m_szName, "FEP_QUI") != 0 || it[3].m_szName[0] != 0)
        return false;

    g_startGameTarget = it[0].m_nTargetMenu;
    g_optionsTarget   = it[1].m_nTargetMenu;
    g_quitTarget      = it[2].m_nTargetMenu;

    MenuItem cont = it[0];                       // inherits type, alignment and the 320,170 anchor
    strcpy_s(cont.m_szName, KEY_CONTINUE);
    cont.m_nTargetMenu = SCREEN_LOAD_FIRST_SAVE;

    // Items 1..3 keep X=Y=0 ("below the previous item"), exactly like the original items 2/3.
    it[3] = it[2];
    it[2] = it[1];
    it[1] = it[0];
    it[0] = cont;
    for (int i = 1; i <= 3; i++) it[i].m_X = it[i].m_Y = 0;
    s.m_nStartEntry = 0;
    return true;
}

// Some plugins relocate the frontend screen table (copy it to the heap and rewrite the operands in the
// menu code), so the game no longer reads it at 0x8CE008. The code operands are the authority: the
// instructions in DrawStandardMenus and JumpToGenericMessageScreen that load the table base hold the
// address in use, and are read instead of assuming the original location.
constexpr uintptr_t ADDR_TableBaseOperandDraw = 0x579568; // DrawStandardMenus
constexpr uintptr_t ADDR_TableBaseOperandMsg  = 0x576B38; // JumpToGenericMessageScreen

static MenuScreen* g_table = nullptr; // the screen table the game actually uses

static MenuScreen* ResolveScreenTable() {
    const uintptr_t a = *reinterpret_cast<const uint32_t*>(ADDR_TableBaseOperandDraw);
    const uintptr_t b = *reinterpret_cast<const uint32_t*>(ADDR_TableBaseOperandMsg);
    LOG_DEBUG("screen table base in menu code: %08X (draw) / %08X (message), original %08X", (unsigned)a, (unsigned)b, (unsigned)ADDR_aScreens);
    return reinterpret_cast<MenuScreen*>(a == b ? a : ADDR_aScreens);
}

// Both entries need the text hook and the table; set them up once, whichever is needed first.
static bool g_textHooked = false;
static bool EnsureTextHook() {
    if (!g_textHooked) g_textHooked = HookMenuTextCalls() > 0;
    return g_textHooked;
}
static MenuScreen* EnsureTable() {
    if (!g_table) g_table = ResolveScreenTable();
    return g_table;
}

// ---------------------------------------------------------------------------
// Pause menu: insert "Load latest save" right below "Resume"
// ---------------------------------------------------------------------------
static bool g_pauseInserted = false;
static int  g_pauseSlot     = -1;
static int8_t g_prevPauseScreen = -1;

static bool InsertLoadLatestEntry(MenuScreen& s) {
    MenuItem* it = s.m_aItems;

    // Expect exactly: Resume, Save/Load game, Map, Stats, Brief, Options, Quit Game, <empty>.
    if (strcmp(s.m_szTitleName, "FET_PAU") != 0 || strcmp(it[0].m_szName, "FEP_RES") != 0 ||
        strcmp(it[1].m_szName, "FEH_SGA") != 0 || strcmp(it[6].m_szName, "FEP_QUI") != 0 || it[7].m_szName[0] != 0)
        return false;

    MenuItem entry = it[1];                      // same type and alignment; X=Y=0 = "below the previous item"
    strcpy_s(entry.m_szName, KEY_LOADLATEST);
    // With the confirmation the game asks "all unsaved progress will be lost" first; its "Yes" starts the load.
    entry.m_nTargetMenu = g_pauseConfirm ? SCREEN_LOAD_GAME_ASK : SCREEN_LOAD_FIRST_SAVE;

    for (int i = 7; i >= 2; --i) it[i] = it[i - 1];
    it[1] = entry;
    // The game lays out items with X=Y=0 under their predecessor on the first draw and writes the result back, so
    // after the pause menu was drawn once the items have positions. Reset everything below "Resume" (the anchor)
    // so that it is laid out again with the new entry; otherwise two entries would sit on the same spot.
    for (int i = 1; i <= 7; ++i) it[i].m_X = it[i].m_Y = 0;
    return true;
}

// Runs while the game is in a session (not in the main menu).
static void PauseMenuTick(MenuManager& m) {
    const bool onPause = m.m_nCurrentScreen == SCREEN_PAUSE_MENU;
    if (onPause && g_prevPauseScreen != SCREEN_PAUSE_MENU) {
        // The pause menu was just opened: the newest save can have changed since (the player saved).
        g_pauseSlot = FindNewestSlot();
        if (g_pauseSlot >= 0 && !g_pauseInserted) {
            g_pauseInserted = EnsureTextHook() && InsertLoadLatestEntry(EnsureTable()[SCREEN_PAUSE_MENU]);
            Log("pause menu entry inserted: %d (newest slot %d)", g_pauseInserted, g_pauseSlot);
            if (!g_pauseInserted) g_pauseLoadLatest = false;   // the table is not what we expect: never retry
        }
    }
    if (onPause && g_pauseInserted && g_pauseSlot >= 0) m.m_SelectedSlot = static_cast<uint8_t>(g_pauseSlot);
    g_prevPauseScreen = m.m_nCurrentScreen;
}

// ---------------------------------------------------------------------------
// Settings page: Options > QuickContinue
// ---------------------------------------------------------------------------
// The ini holds the defaults. Changes made on the page are written to QuickContinue.SA.cfg next to the save games and
// win over the ini (read at startup). With SettingsMenu=0 there is no page and the cfg is neither read nor written.
// Only the settings that players switch in daily use are on the page; the others (SkipIntro, SkipTitleFade,
// SmoothLoadScreen, SkipKey, Language, [Text], Debug) stay in the ini and are not touched by the cfg.
// Every setting is read once at startup, so a change applies from the next start of the game.
struct Setting { const char* key; bool* value; bool def; };   // def: used if the ini has no such key
static const Setting kSettings[] = {
    { "ContinueButton",      &g_continueButton, true },
    { "AutoLoad",            &g_autoLoad, false },
    { "PauseMenuLoadLatest", &g_pauseLoadLatest, true },
    { "PauseMenuConfirm",    &g_pauseConfirm, true },
};
constexpr int kSettingCount = sizeof(kSettings) / sizeof(kSettings[0]);

// Texts of the page, per language (UTF-8, converted by Gxt). Languages that have no entry here show English.
// Only letters of the game font are used.
struct PageText {
    const char* code;
    const char* page;                    // name of the page; the entry in the Options list shows it in capitals
    const char* names[kSettingCount];    // ContinueButton, AutoLoad, PauseMenuLoadLatest, PauseMenuConfirm
    const char* note;
    const char* reset;
};
static const PageText kPageTexts[] = {
    { "en", "Quick start", { "Continue button", "Load save on start", "Pause: load latest save", "Pause: ask first" },
      "Changes apply after restarting the game", "Reset to defaults" },
    { "de", "Schnellstart", { "Fortsetzen-Knopf", "Beim Start laden", "Pause: Letzten Stand laden", "Pause: vorher fragen" },
      "Änderungen gelten nach einem Neustart", "Auf Standard zurücksetzen" },
    { "fr", "Démarrage rapide", { "Bouton Continuer", "Charger au démarrage", "Pause : dernière partie", "Pause : demander" },
      "Valable après redémarrage", "Valeurs par défaut" },
    { "it", "Avvio rapido", { "Pulsante Continua", "Carica all'avvio", "Pausa: ultimo salvataggio", "Pausa: chiedi prima" },
      "Valido dopo il riavvio", "Ripristina predefiniti" },
    { "es", "Inicio rápido", { "Botón Continuar", "Cargar al iniciar", "Pausa: última partida", "Pausa: preguntar" },
      "Se aplica al reiniciar", "Restablecer valores" },
    { "pt", "Início rápido", { "Continuar no menu", "Carregar ao iniciar", "Pausa: carregar último", "Pausa: perguntar antes" },
      "Muda ao reiniciar o jogo", "Valores predefinidos" },
    { "nl", "Snelstart", { "Doorgaan-knop", "Laden bij start", "Pauze: laatste spel laden", "Pauze: eerst vragen" },
      "Wijzigingen gelden na herstart", "Standaardwaarden" },
    { "sv", "Snabbstart", { "Fortsätt-knapp", "Ladda vid start", "Paus: ladda senaste", "Paus: bekräfta först" },
      "Ändringar gäller efter omstart", "Standardvärden" },
    { "fi", "Pikakäynnistys", { "Jatka-painike", "Lataa käynnistyksessä", "Tauko: lataa viimeisin", "Tauko: kysy ensin" },
      "Voimassa uudelleenkäynnistyksen jälkeen", "Oletusarvot" },
    { "hu", "Gyorsindítás", { "Folytatás gomb", "Betöltés induláskor", "Szünet: utolsó betöltése", "Szünet: rákérdezés" },
      "Újraindítás után érvényes", "Alapértékek" },
};

static const PageText& PageTexts() {
    const char* c = CurrentLang().code;
    for (const PageText& t : kPageTexts) if (strcmp(t.code, c) == 0) return t;
    return kPageTexts[0];
}

// [Text] in the ini: own wording for the page (any language); empty = the built-in text of the language.
static char g_textPage[48], g_textNames[kSettingCount][48], g_textNote[96], g_textReset[48];

static void LoadPageTextOverrides(const char* ini) {
    GetPrivateProfileStringA("Text", "PageName", "", g_textPage, sizeof(g_textPage), ini);
    static const char* const keys[kSettingCount] = { "ContinueButton", "AutoLoad", "PauseLoadLatest", "PauseAskFirst" };
    for (int i = 0; i < kSettingCount; i++)
        GetPrivateProfileStringA("Text", keys[i], "", g_textNames[i], sizeof(g_textNames[i]), ini);
    GetPrivateProfileStringA("Text", "RestartNote", "", g_textNote, sizeof(g_textNote), ini);
    GetPrivateProfileStringA("Text", "ResetButton", "", g_textReset, sizeof(g_textReset), ini);
}

// Capitals for the Options list: ASCII letters and the accented letters of the game font (codes 0x97-0xAD).
static void UpperGxt(char* t) {
    for (unsigned char* p = reinterpret_cast<unsigned char*>(t); *p; p++) {
        if (*p >= 'a' && *p <= 'z') *p -= 32;
        else if (*p >= 0x97 && *p <= 0xAC) *p -= 0x17;
        else if (*p == 0xAD) *p = 0xAE;
    }
}
static void UpperAscii(char* t) { for (; *t; t++) if (*t >= 'a' && *t <= 'z') *t -= 32; }

static void WriteSavedSettings() {
    if (!g_settingsMenu || !g_cfgPath[0]) return;
    for (const Setting& st : kSettings) WritePrivateProfileStringA("QuickContinue", st.key, *st.value ? "1" : "0", g_cfgPath);
}

// "Reset to defaults": delete the cfg and take the values of the ini again.
static void ResetSettings() {
    if (g_cfgPath[0]) DeleteFileA(g_cfgPath);
    for (const Setting& st : kSettings)
        *st.value = GetPrivateProfileIntA("QuickContinue", st.key, st.def ? 1 : 0, g_iniPath) != 0;
    Log("settings reset to the ini values");
}

static void LoadSavedSettings() {
    if (!g_settingsMenu) return;
    char docs[MAX_PATH];
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs))) return;
    _snprintf_s(g_cfgPath, MAX_PATH, _TRUNCATE, "%s\\GTA San Andreas User Files\\QuickContinue.SA.cfg", docs);
    for (const Setting& st : kSettings)   // the ini value stays if the cfg has none
        *st.value = GetPrivateProfileIntA("QuickContinue", st.key, *st.value ? 1 : 0, g_cfgPath) != 0;
    Log("settings: saved values in %s", g_cfgPath);
}

// Texts for QC_SET (entry in Options), QC_STT (page title), QC_S0.. (a setting with its value) and QC_NOTE.
static const char* SettingsText(void* /*self*/, const char* key) {
    const PageText& t = PageTexts();
    static char list[64];
    if (strcmp(key, "QC_SET") == 0) {          // entry in the Options list: capitals like its neighbours
        if (g_textPage[0]) { strcpy_s(list, g_textPage); UpperAscii(list); }
        else { strcpy_s(list, Gxt(t.page)); UpperGxt(list); }
        return list;
    }
    if (strcmp(key, "QC_STT") == 0) return g_textPage[0] ? g_textPage : Gxt(t.page);   // page title: as written
    if (strcmp(key, "QC_NOTE") == 0) return g_textNote[0] ? g_textNote : Gxt(t.note);
    if (strcmp(key, "QC_RST") == 0) { g_rowPending = kSettingCount; return g_textReset[0] ? g_textReset : Gxt(t.reset); }
    if (key[3] == 'S' && key[4] >= '0' && key[4] < '0' + kSettingCount && key[5] == 0) {
        g_rowPending = key[4] - '0';   // the game asks for the value column right after the name
        return g_textNames[g_rowPending][0] ? g_textNames[g_rowPending] : Gxt(t.names[g_rowPending]);
    }
    return nullptr;
}

// The value column of a row: the game requests "FEL_xxx" (a language name) for it. Returns the setting's ON / OFF
// (the game's own translated texts), or nothing for the reset row.
static const char* SettingsValueText(void* self) {
    const int i = g_rowPending;
    g_rowPending = -1;
    if (i >= kSettingCount) return "";
    const char* v = g_textGetOrig(self, *kSettings[i].value ? "FEM_ON" : "FEM_OFF");
    return (v && v[0]) ? v : (*kSettings[i].value ? "On" : "Off");
}

constexpr int kFirstRow = 1;   // item 0 of the page is the note

static bool g_settingsTried    = false;
static bool g_settingsInserted = false;

static void SetItem(MenuItem& it, uint8_t action, const char* name, uint8_t type, int8_t target) {
    it.m_nActionType = action;
    memset(it.m_szName, 0, sizeof(it.m_szName));
    strcpy_s(it.m_szName, name);
    it.m_nType = type;
    it.m_nTargetMenu = target;
}

// Adds "QuickContinue" to Options (in front of Back) and builds the page in the borrowed console screen.
static bool InstallSettingsMenu(MenuScreen* table) {
    MenuScreen& opt  = table[SCREEN_OPTIONS];
    MenuScreen& disp = table[SCREEN_DISPLAY];
    MenuScreen& page = table[SCREEN_GAME_LOADED];

    if (strcmp(opt.m_szTitleName, "FET_OPT") != 0 || strcmp(disp.m_szTitleName, "FEH_DIS") != 0 ||
        strcmp(page.m_szTitleName, "FET_LG") != 0 || strcmp(page.m_aItems[1].m_szName, "FEM_OK") != 0 ||
        strcmp(disp.m_aItems[0].m_szName, "FED_BRI") != 0 || strcmp(disp.m_aItems[1].m_szName, "MAP_LEG") != 0)
        return false;

    int back = -1;                                           // the "Back" entry (absolute position) closes the list
    for (int i = 0; i < 11; i++) if (strcmp(opt.m_aItems[i].m_szName, "FEDS_TB") == 0) { back = i; break; }
    if (back < 1 || opt.m_aItems[11].m_szName[0] != 0) return false;
    const MenuItem backItem = opt.m_aItems[back];

    // The page: the first row takes the anchor of the Display page's first row, the others stack below it.
    memset(&page, 0, sizeof(page));
    strcpy_s(page.m_szTitleName, "QC_STT");
    page.m_nParentMenu = opt.m_nParentMenu;
    page.m_nStartEntry = 1;                                   // the cursor starts on the first setting, not on the note
    // Row 0 is a plain text line (action LABEL, type 0) like the heading lines of the game's own pages: it cannot be selected.
    SetItem(page.m_aItems[0], ACTION_LABEL, "QC_NOTE", 0, 0);
    for (int i = 0; i < kSettingCount; i++) {
        MenuItem& it = page.m_aItems[kFirstRow + i];
        it = disp.m_aItems[i == 0 ? 0 : 1];                   // option row: type and alignment; the first one keeps the anchor
        char key[8]; _snprintf_s(key, sizeof(key), _TRUNCATE, "QC_S%d", i);
        SetItem(it, ACTION_QC_SETTING, key, TYPE_OPTION, SCREEN_GAME_LOADED);
        if (i) it.m_X = it.m_Y = 0;
    }
    MenuItem& reset = page.m_aItems[kFirstRow + kSettingCount];
    reset = disp.m_aItems[1];
    SetItem(reset, ACTION_QC_SETTING, "QC_RST", TYPE_OPTION, SCREEN_GAME_LOADED);
    reset.m_X = reset.m_Y = 0;
    page.m_aItems[kFirstRow + kSettingCount + 1] = backItem;

    // Options: the new entry is copied from the one above Back (same type, stacked below it).
    MenuItem entry = opt.m_aItems[back - 1];
    SetItem(entry, ACTION_CHANGEMENU, "QC_SET", TYPE_BUTTON, SCREEN_GAME_LOADED);
    entry.m_X = entry.m_Y = 0;
    for (int i = 10; i >= back; --i) opt.m_aItems[i + 1] = opt.m_aItems[i];
    opt.m_aItems[back] = entry;
    return true;
}

// ProcessPCMenuOptions is the first step of the game's option handling; returning 1 means "handled".
using PcOptionsFn = char(__thiscall*)(void* self, char input, char enter);
static PcOptionsFn g_pcOptionsOrig = nullptr;

static char __fastcall PcOptionsHook(MenuManager* self, void* /*edx*/, char input, char enter) {
    if (g_settingsInserted && self->m_nCurrentScreen == SCREEN_GAME_LOADED && (input || enter)) {
        const int item = self->m_nCurrentScreenItem;
        const int sel  = item - kFirstRow;   // 0..kSettingCount-1 = a setting, kSettingCount = reset
        if (sel >= 0 && sel <= kSettingCount && g_table[SCREEN_GAME_LOADED].m_aItems[item].m_nActionType == ACTION_QC_SETTING) {
            if (sel == kSettingCount) {
                if (enter) ResetSettings();   // the reset row is a button: Enter only
            } else {
                *kSettings[sel].value = !*kSettings[sel].value;
                WriteSavedSettings();
                LOG_DEBUG("setting %s = %d", kSettings[sel].key, *kSettings[sel].value);
            }
            return 1;
        }
    }
    return g_pcOptionsOrig(self, input, enter);
}

static void SetupSettingsMenu() {
    g_settingsTried = true;
    if (!EnsureTextHook()) { Log("settings page: text hook failed"); return; }
    const uintptr_t target = CheckCallSite(ADDR_PcOptionsCall, 0);
    if (!target) { Log("settings page: option handler call not found"); return; }
    if (!InstallSettingsMenu(EnsureTable())) { Log("settings page: screen tables are not as expected, page skipped"); return; }
    g_pcOptionsOrig = reinterpret_cast<PcOptionsFn>(target);
    PatchRel(ADDR_PcOptionsCall, static_cast<int32_t>(reinterpret_cast<uintptr_t>(&PcOptionsHook) - (ADDR_PcOptionsCall + 5)));
    g_settingsInserted = true;
    Log("settings page installed (Options > QuickContinue)");
}

// ---------------------------------------------------------------------------
// Per-frame hook (menu draw)
// ---------------------------------------------------------------------------
static int  g_slot       = -1;
static bool g_initDone   = false;
static int8_t g_prevScreen = -1;

static void PrimeLoadingScreen();

static void StartLoad(int slot) {
    MenuManager& m = Menu();
    m.m_bDontDrawFrontEnd   = false;
    m.m_SelectedSlot        = static_cast<uint8_t>(slot);
    MissionPackGame()       = 0;
    m.m_nCurrentScreenItem  = 0;
    m.m_nCurrentScreen      = SCREEN_LOAD_FIRST_SAVE;
    m.m_CurrentlyLoading    = true;
    PrimeLoadingScreen();
}

static void FirstMenuFrame() {
    g_initDone = true;
    g_slot = FindNewestSlot();
    Log("first main-menu frame: newest slot = %d (0-based, -1 = none)", g_slot);
    if (g_slot < 0) return; // no save game: leave the menu alone

    if (g_continueButton) {
        const bool hooked = EnsureTextHook();
        const bool inserted = InsertContinueEntry(EnsureTable()[SCREEN_MAIN_MENU]);
        Log("text hook installed: %d, main menu entry inserted: %d", hooked, inserted);
        g_menuPatched = hooked && inserted;
    }
    if (g_autoLoad && !(g_skipKey && (GetAsyncKeyState(g_skipKey) & 0x8000))) StartLoad(g_slot);
}

static void DumpMainMenu(const char* when) {
    MenuScreen& s = g_table[SCREEN_MAIN_MENU];
    Log("main menu (%s): selected item %d", when, Menu().m_nCurrentScreenItem);
    for (int i = 0; i < 5; i++) {
        const MenuItem& it = s.m_aItems[i];
        Log("  [%d] action %u name '%.8s' type %u target %d pos %u,%u align %u", i, it.m_nActionType, it.m_szName,
            it.m_nType, it.m_nTargetMenu, it.m_X, it.m_Y, it.m_nAlign);
    }
}

// Debug=1: logs every change of the menu/loading flags, so a failing load can be followed in the log.
static void TraceState(const MenuManager& m) {
    static int  screen = -99, loadingData = -1, curLoading = -1, menuActive = -1, startLoading = -1, slot = -1, gs = -1, item = -99;
    const int   state = *reinterpret_cast<int*>(0xC8D4C0);
    if (screen == m.m_nCurrentScreen && loadingData == m.m_bLoadingData && curLoading == m.m_CurrentlyLoading &&
        menuActive == m.m_bMenuActive && startLoading == m.m_bStartGameLoading && slot == m.m_SelectedSlot &&
        gs == state && item == m.m_nCurrentScreenItem)
        return;
    screen = m.m_nCurrentScreen; loadingData = m.m_bLoadingData; curLoading = m.m_CurrentlyLoading; menuActive = m.m_bMenuActive;
    startLoading = m.m_bStartGameLoading; slot = m.m_SelectedSlot; gs = state; item = m.m_nCurrentScreenItem;
    Log("[%lu] screen %d item %d slot %d | menuActive %d currentlyLoading %d loadingData %d startGameLoading %d | gGameState %d",
        GetTickCount() % 100000, screen, item, slot, menuActive, curLoading, loadingData, startLoading, state);
}

static void MenuTick() {
    MenuManager& m = Menu();
    if (g_debug && g_initDone) TraceState(m);
    static int frames = 0;
    if (g_debug && g_menuPatched && m.m_nCurrentScreen == SCREEN_MAIN_MENU && ++frames == 60) DumpMainMenu("after 60 frames");
    if (g_settingsMenu && !g_settingsTried) SetupSettingsMenu();
    static bool logged = false;
    if (!logged) { logged = true; LOG_DEBUG("menu hook running, m_bMainMenuSwitch = %d, screen = %d", m.m_bMainMenuSwitch, m.m_nCurrentScreen); }
    if (!m.m_bMainMenuSwitch) {                  // pause menu / in-game
        if (g_pauseLoadLatest) PauseMenuTick(m);
        return;
    }
    if (!g_initDone) FirstMenuFrame();
    if (!g_menuPatched) return;

    // "Continue" was clicked: the game is about to start loading.
    if (m.m_nCurrentScreen == SCREEN_LOAD_FIRST_SAVE) PrimeLoadingScreen();

    // The slot used by "Continue" is read when the load screen starts; keep it current.
    if (m.m_nCurrentScreen == SCREEN_MAIN_MENU) {
        m.m_SelectedSlot = static_cast<uint8_t>(g_slot);

        // SwitchToNewScreen(GO_BACK) re-selects item 0/1/2 by hard-coded index for Start/Options/Quit;
        // our extra entry shifts them by one.
        if (g_prevScreen != SCREEN_MAIN_MENU &&
            (g_prevScreen == g_startGameTarget || g_prevScreen == g_optionsTarget || g_prevScreen == g_quitTarget) &&
            m.m_nCurrentScreenItem >= 0 && m.m_nCurrentScreenItem <= 2)
            m.m_nCurrentScreenItem++;
    }
    g_prevScreen = m.m_nCurrentScreen;
}

using MenuFn = void(__thiscall*)(void*);
static MenuFn g_menuOrig[2] = {};   // per call site: the sites may have been redirected to different targets

static void RunTick() {
    __try {
        MenuTick();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        g_initDone = true; g_menuPatched = false; // never retry after a fault
    }
}
static void __fastcall MenuHook0(void* self, void* /*edx*/) { g_menuOrig[0](self); RunTick(); }
static void __fastcall MenuHook1(void* self, void* /*edx*/) { g_menuOrig[1](self); RunTick(); }

static bool Install() {
    const uint32_t probe = *reinterpret_cast<uint32_t*>(ADDR_VersionProbe);
    Log("version probe at 0x401000 = %08X", probe);
    if (probe != 0x53EC8B55 && probe != 0x16197BE9) return false; // not GTA SA v1.0 US

    // Another plugin (widescreen fix, SilentPatch...) may already have redirected these calls, so any
    // call target is accepted and chained to; the thiscall(this) shape is unchanged either way.
    void* const hooks[2] = { reinterpret_cast<void*>(&MenuHook0), reinterpret_cast<void*>(&MenuHook1) };
    bool any = false;
    for (int i = 0; i < 2; i++) {
        const uintptr_t site = ADDR_MenuDrawCalls[i];
        const uintptr_t target = CheckCallSite(site, 0);
        Log("menu draw site %08X: opcode %02X, call target %08X%s", (unsigned)site, *reinterpret_cast<uint8_t*>(site),
            (unsigned)target, target == ADDR_MenuDraw ? " (DrawFrontEnd)" : " (redirected by another plugin)");
        if (!target) continue;
        g_menuOrig[i] = reinterpret_cast<MenuFn>(target);
        PatchRel(site, static_cast<int32_t>(reinterpret_cast<uintptr_t>(hooks[i]) - (site + 5)));
        any = true;
    }
    return any;
}

// ---------------------------------------------------------------------------
// Startup: skip splash screens / intro videos / the title fade
// ---------------------------------------------------------------------------
// The game starts in gGameState 0 (splashes) and walks through logo video, title video and the
// copyright splash up to state 5 (FRONTEND_LOADING). psInitialize writes the initial 0 with
// `mov dword ptr [gGameState], 0`; its immediate is changed to 5 so the whole intro is skipped.
constexpr uintptr_t ADDR_WinMainBegin     = 0x748000; // WinMain / psInitialize / game state machine
constexpr uintptr_t ADDR_WinMainEnd       = 0x74A000;
constexpr uintptr_t ADDR_TitleFadeIn      = 0x590860; // CLoadingScreen::DoPCTitleFadeIn
constexpr uintptr_t ADDR_TitleFadeOut     = 0x590990; // CLoadingScreen::DoPCTitleFadeOut

// The state machine in WinMain stores the next state with `mov dword ptr [gGameState], imm32`:
//   after Logo.mpg was started:      2 (PLAYING_LOGO)  -> 3 (TITLE)
//   after GTAtitles.mpg was started: 4 (PLAYING_INTRO) -> 5 (FRONTEND_LOADING)
// i.e. it goes straight to the next state without playing the video. Starting the videos is
// disabled as well (VideoPlayer::Play, cdecl void) so no frame/sound of them leaks through.
constexpr uintptr_t ADDR_VideoPlay = 0x747660;

static void SkipIntro() {
    // mov dword ptr [0xC8D4C0], imm32   =  C7 05 C0 D4 C8 00 <imm32>
    static const uint8_t pat[] = { 0xC7, 0x05, 0xC0, 0xD4, 0xC8, 0x00 };
    int patched = 0;
    for (uintptr_t a = ADDR_WinMainBegin; a < ADDR_WinMainEnd - 10; a++) {
        if (memcmp(reinterpret_cast<void*>(a), pat, sizeof(pat)) != 0) continue;
        const uint32_t imm = *reinterpret_cast<uint32_t*>(a + 6);
        LOG_DEBUG("gGameState store at %08X, immediate %u", (unsigned)a, imm);
        uint32_t repl = 0;
        if (imm == 2) repl = 3;
        else if (imm == 4) repl = 5;
        if (!repl) continue;
        WriteMem(a + 6, &repl, 4);
        patched++;
    }
    Log("intro skip: patched %d state transition(s) (expected 2)", patched);
    if (patched != 2) return;

    const uint8_t first = *reinterpret_cast<uint8_t*>(ADDR_VideoPlay);
    LOG_DEBUG("VideoPlayer::Play first byte %02X", first);
    const uint8_t ret = 0xC3;
    WriteMem(ADDR_VideoPlay, &ret, 1);
}

// DoPCTitleFadeIn/Out do more than fade: they also leave CLoadingScreen in the state the loading
// screen later relies on (current splash = copyright, fade-in-from-black flag...). Removing the calls
// left m_currDisplayedSplash at NONE and the in-game loading screen then read a bad splash entry.
// The calls are therefore redirected to stubs that only set the state the originals leave behind.
constexpr uintptr_t ADDR_LS_currDisplayedSplash = 0x8D093C; // int32, -1 = none, 0 = copyright
constexpr uintptr_t ADDR_LS_bFading             = 0xBAB31C;
constexpr uintptr_t ADDR_LS_bFadeInNextFromBlack= 0xBAB31E;
constexpr uintptr_t ADDR_LS_FadeAlpha           = 0xBAB320;

static void __cdecl SkippedTitleFadeIn() {
    *reinterpret_cast<int32_t*>(ADDR_LS_currDisplayedSplash) = 0;
    *reinterpret_cast<bool*>(ADDR_LS_bFadeInNextFromBlack)   = true;
    *reinterpret_cast<bool*>(ADDR_LS_bFading)                = false;
    *reinterpret_cast<uint8_t*>(ADDR_LS_FadeAlpha)           = 255;
}

static void __cdecl SkippedTitleFadeOut() {
    *reinterpret_cast<int32_t*>(ADDR_LS_currDisplayedSplash) = 0;
    *reinterpret_cast<bool*>(ADDR_LS_bFadeInNextFromBlack)   = true;
    *reinterpret_cast<bool*>(ADDR_LS_bFading)                = true;
    *reinterpret_cast<uint8_t*>(ADDR_LS_FadeAlpha)           = 5;
}

constexpr uintptr_t ADDR_LS_bFadeOutToBlack     = 0xBAB31F;
constexpr uintptr_t ADDR_LS_timeSinceLastScreen = 0xBAB340; // float

// The first loading splash normally fades in from black over ~70 frames (20 black + 50 fading), which
// the copyright image used to hide. Starting the loading screen directly on splash 1, fully visible, with
// the "time since last splash" far in the past-negative so it does not switch splashes early, skips that.
static void PrimeLoadingScreen() {
    static bool done = false;
    if (done || !g_smoothLoad) return;
    done = true;
    *reinterpret_cast<int32_t*>(ADDR_LS_currDisplayedSplash)  = 1;      // SPLASH_1_NO_BAR
    *reinterpret_cast<bool*>(ADDR_LS_bFading)                 = false;
    *reinterpret_cast<bool*>(ADDR_LS_bFadeInNextFromBlack)    = false;
    *reinterpret_cast<bool*>(ADDR_LS_bFadeOutToBlack)         = false;
    *reinterpret_cast<uint8_t*>(ADDR_LS_FadeAlpha)            = 255;
    *reinterpret_cast<float*>(ADDR_LS_timeSinceLastScreen)    = 1.0e9f; // delta = now - 1e9 < 0: keep splash 1
    Log("loading screen primed (splash 1, no fade-in)");
}

static void SkipTitleFade() {
    int n = 0;
    for (uintptr_t a = ADDR_WinMainBegin; a < ADDR_WinMainEnd - 5; a++) {
        void* stub = nullptr;
        if (CheckCallSite(a, ADDR_TitleFadeIn))       stub = reinterpret_cast<void*>(&SkippedTitleFadeIn);
        else if (CheckCallSite(a, ADDR_TitleFadeOut)) stub = reinterpret_cast<void*>(&SkippedTitleFadeOut);
        if (!stub) continue;
        PatchRel(a, static_cast<int32_t>(reinterpret_cast<uintptr_t>(stub) - (a + 5)));
        n++;
    }
    Log("title fade skip: redirected %d call(s)", n);
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(mod);
        LoadSettings(mod);
        if (Install()) {
            if (g_skipIntro) SkipIntro();
            if (g_skipTitleFade) SkipTitleFade();
        }
    }
    return TRUE;
}
