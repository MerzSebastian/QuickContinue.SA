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
    if (!g_logPath[0]) return;
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

static void LoadSettings(HMODULE self) {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(self, path, MAX_PATH)) return;
    char* dot = strrchr(path, '.');
    if (!dot) return;
    // The game folder is often not writable for the game process: log into the user files folder instead.
    char docs[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, docs))) {
        _snprintf_s(g_logPath, MAX_PATH, _TRUNCATE, "%s\\GTA San Andreas User Files\\QuickContinue.SA.log", docs);
        remove(g_logPath);
    }
    strcpy_s(dot, path + MAX_PATH - dot, ".ini");
    g_debug          = GetPrivateProfileIntA("QuickContinue", "Debug", 0, path) != 0;
    Log("QuickContinue.SA %s loaded from %s", kVersion, path);
    g_continueButton = GetPrivateProfileIntA("QuickContinue", "ContinueButton", 1, path) != 0;
    g_autoLoad       = GetPrivateProfileIntA("QuickContinue", "AutoLoad", 0, path) != 0;
    g_skipKey        = GetPrivateProfileIntA("QuickContinue", "SkipKey", VK_CONTROL, path);
    g_skipIntro      = GetPrivateProfileIntA("QuickContinue", "SkipIntro", 1, path) != 0;
    g_skipTitleFade  = GetPrivateProfileIntA("QuickContinue", "SkipTitleFade", 1, path) != 0;
    g_smoothLoad     = GetPrivateProfileIntA("QuickContinue", "SmoothLoadScreen", 1, path) != 0;
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
static const char KEY_CONTINUE[] = "QC_CONT";
static char       g_label[32]    = "Continue";

static const char* LabelForLanguage(uint8_t lang) {
    switch (lang) {
    case GXT_LANG_FRENCH:  return "Continuer";
    case GXT_LANG_GERMAN:  return "Fortsetzen";
    case GXT_LANG_ITALIAN: return "Continua";
    case GXT_LANG_SPANISH: return "Continuar";
    default:           return "Continue";
    }
}

using TextGetFn = const char*(__thiscall*)(void* self, const char* key);
static TextGetFn g_textGetOrig = nullptr;

static const char* __fastcall TextGetHook(void* self, void* /*edx*/, const char* key) {
    if (key && key[0] == 'Q' && strcmp(key, KEY_CONTINUE) == 0) {
        strcpy_s(g_label, LabelForLanguage(Menu().m_nPrefsLanguage));
        return g_label; // GxtChar is plain ASCII for these characters
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
        const int textCalls = HookMenuTextCalls();
        g_table = ResolveScreenTable();
        const bool inserted = InsertContinueEntry(g_table[SCREEN_MAIN_MENU]);
        Log("CText::Get call sites hooked: %d, menu entry inserted: %d", textCalls, inserted);
        g_menuPatched = textCalls > 0 && inserted;
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
    static bool logged = false;
    if (!logged) { logged = true; LOG_DEBUG("menu hook running, m_bMainMenuSwitch = %d, screen = %d", m.m_bMainMenuSwitch, m.m_nCurrentScreen); }
    if (!m.m_bMainMenuSwitch) return;            // pause menu / in-game
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
