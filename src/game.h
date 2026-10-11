// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Sebastian Merz
// Game definitions for GTA San Andreas v1.0 US (32-bit).
// Struct layouts and addresses of the frontend menu. The layouts are checked by the
// static_asserts below against their known sizes.
#pragma once
#include <cstddef>
#include <cstdint>

namespace game {

// ---------------------------------------------------------------------------
// CMenuManager (size 0x1B78)
// ---------------------------------------------------------------------------
struct MPack { uint8_t m_Id; char m_Name[260]; };

struct MenuManager {
    int8_t   m_nStatsScrollDirection;
    float    m_fStatsScrollSpeed;
    uint8_t  m_nSelectedRow;
    int32_t  m_CurrentGallerySelected, m_CurrentMaxGalleryImages, m_CurrentMaxRealPhotos, m_CurrentSelectedRealPhoto;
    uint32_t m_ReloadImageTime;
    bool     m_PrefsUseVibration, m_bHudOn;
    int32_t  m_nRadarMode;
    int32_t  field_28;
    int32_t  m_nTargetBlipIndex;
    int8_t   m_nSysMenu;
    bool     m_DisplayControllerOnFoot;
    bool     m_bDontDrawFrontEnd;
    bool     m_bActivateMenuNextFrame;
    bool     m_bMenuAccessWidescreen;
    bool     field_35;
    int32_t  m_KeyPressedCode;
    int32_t  m_PrefsBrightness;
    float    m_fDrawDistance;
    bool     m_bShowSubtitles;
    bool     m_abPrefsMapBlips[5];
    bool     m_bMapLegend, m_bWidescreenOn, m_bPrefsFrameLimiter, m_bRadioAutoSelect, m_PrefsAudioOutputMode;
    int8_t   m_nSfxVolume, m_nRadioVolume;
    bool     m_bRadioEq;
    int8_t   m_nRadioStation;
    bool     m_RecheckNumPhotos;
    int32_t  m_nCurrentScreenItem;
    bool     m_bQuitGameNoDVD;
    bool     m_bDrawingMap, m_bStreamingDisabled, m_bAllStreamingStuffLoaded;
    bool     m_bMenuActive;
    bool     m_bStartGameLoading;
    int8_t   m_nGameState;
    bool     m_bIsSaveDone;
    bool     m_bLoadingData;
    float    m_fMapZoom;
    float    m_vMapOrigin[2];
    float    m_vMousePos[2];
    bool     m_bMapLoaded;
    int32_t  m_nTitleLanguage, m_nTextLanguage;
    uint8_t  m_nPrefsLanguage, m_nPreviousLanguage;
    int32_t  m_SystemLanguage;
    bool     m_LoadedLanguage;
    int32_t  m_ListSelection, m_RenderScreenOnce;
    uint8_t* m_GalleryImgBuffer;
    void*    m_GpJpgTex;
    bool     m_StartUpFrontEndRequestedForPads;
    int32_t  m_ScreenXOffset, m_ScreenYOffset;
    uint32_t m_UserTrackIndex;
    int8_t   m_RadioMode;
    bool     m_bInvertPadX1, m_bInvertPadY1, m_bInvertPadX2, m_bInvertPadY2, m_bSwapPadAxis1, m_bSwapPadAxis2;
    uint8_t  m_RedefiningControls;
    bool     m_DisplayTheMouse;
    int32_t  m_nMousePosX, m_nMousePosY;
    bool     m_bPrefsMipMapping, m_bTracksAutoScan;
    int32_t  m_nPrefsAntialiasing, m_nDisplayAntialiasing;
    int8_t   m_ControlMethod;
    int32_t  m_nPrefsVideoMode, m_nDisplayVideoMode, m_nCurrentRwSubsystem;
    int32_t  m_nMousePosWinX, m_nMousePosWinY;
    bool     m_bSavePhotos;
    bool     m_bMainMenuSwitch;
    uint8_t  m_nPlayerNumber;
    bool     m_bLanguageChanged;
    int32_t  field_EC;
    void*    m_pPressedKey;
    bool     m_isPreInitialised;
    void*    m_aFrontEndSprites[25];
    bool     m_bTexturesLoaded;
    int8_t   m_nCurrentScreen;
    int8_t   m_nPrevScreen;
    uint8_t  m_SelectedSlot;
    uint8_t  m_nMissionPackGameId;
    MPack    m_MissionPacks[25];
    bool     m_bDoVideoModeUpdate;
    int32_t  m_nPressedMouseButton, m_nJustDownJoyButton;
    bool     m_MenuIsAbleToQuit, m_RadioAvailable;
    int8_t   m_ControllerError;
    bool     m_bScanningUserTracks;
    int32_t  m_nHelperTextFadingAlpha;
    bool     m_KeyPressed[5];
    int32_t  m_nOldMousePosX, m_nOldMousePosY;
    int32_t  m_MouseInBounds;
    int32_t  m_CurrentMouseOption;
    bool     m_bJustOpenedControlRedefWindow, m_EditingControlOptions, m_DeleteAllBoundControls, m_DeleteAllNextDefine;
    int32_t  m_OptionToChange, m_OptionProcessing;
    bool     m_CanBeDefined, m_JustExitedRedefine;
    int32_t  m_HelperText;
    uint32_t m_TimeToStopPadShaking;
    bool     m_TexturesSwapped;
    uint8_t  m_nNumberOfMenuOptions;
    uint32_t m_StatsScrollTime;
    bool     m_bViewRadar;
    uint32_t m_RadarVisibilityChangeTime, m_BriefsArrowBlinkTime;
    uint16_t m_StatusDisablePlayerControls;
    uint32_t m_LastActionTime;
    bool     m_CurrentlyLoading, m_CurrentlyDeleting, m_CurrentlySaving;
    uint32_t m_UserTrackScanningTime;
    bool     m_ErrorPendingReset;
    uint32_t m_ErrorStartTime;
    int32_t  field_1B4C;
    int8_t   m_BackgroundSprite;
    bool     m_InputWaitBlink;
    uint32_t m_LastBlinkTime, m_HelperTextUpdatedTime;
    bool     m_OptionFlashColorState;
    uint32_t m_LastHighlightToggleTime, m_LastTransitionTime, m_SlideLeftMoveTime, m_SlideRightMoveTime;
    int32_t  m_nMouseHoverScreen;
    int32_t  field_1B74;
};
static_assert(sizeof(MenuManager) == 0x1B78, "CMenuManager must be 0x1B78 bytes");

// ---------------------------------------------------------------------------
// Frontend screen table (MenuManager_Internal.h: tMenuScreenItem 0x12, tMenuScreen 0xE2)
// ---------------------------------------------------------------------------
struct MenuItem {
    uint8_t  m_nActionType;
    char     m_szName[8];
    uint8_t  m_nType;
    int8_t   m_nTargetMenu;
    uint16_t m_X;
    uint16_t m_Y;
    uint8_t  m_nAlign;
};
struct MenuScreen {
    char     m_szTitleName[8];
    int8_t   m_nParentMenu;
    int8_t   m_nStartEntry;
    MenuItem m_aItems[12];
};
static_assert(sizeof(MenuItem) == 0x12, "menu item must be 0x12 bytes");
static_assert(sizeof(MenuScreen) == 0xE2, "menu screen must be 0xE2 bytes");

// eMenuScreen values
constexpr int8_t SCREEN_START_GAME      = 1;
constexpr int8_t SCREEN_LOAD_GAME_ASK   = 11; // "All unsaved progress in your current game will be lost. Proceed with loading?"
constexpr int8_t SCREEN_LOAD_FIRST_SAVE = 13;
constexpr int8_t SCREEN_MAIN_MENU       = 34;
constexpr int8_t SCREEN_PAUSE_MENU      = 41;
constexpr int8_t SCREEN_DISPLAY         = 4;
constexpr int8_t SCREEN_OPTIONS         = 33;
constexpr int8_t SCREEN_GAME_LOADED     = 21; // "game loaded - OK" page of the consoles; nothing on PC reaches it, so it is borrowed

// Menu actions and entry types
constexpr uint8_t ACTION_LABEL = 1, ACTION_CHANGEMENU = 5, ACTION_SKIP = 20;
// MENUACTION_CHANGE_LANG: the game prints the language name (CText::Get "FEL_xxx") in the value column of such a row.
// The plugin answers that request with the value of its setting; the Enter / left / right input is handled before the game.
constexpr uint8_t ACTION_QC_SETTING = 36;
constexpr uint8_t TYPE_BUTTON = 11, TYPE_OPTION = 12;

constexpr int MAX_SAVEGAME_SLOTS = 8;

// eLanguage
constexpr uint8_t GXT_LANG_FRENCH = 1, GXT_LANG_GERMAN = 2, GXT_LANG_ITALIAN = 3, GXT_LANG_SPANISH = 4;

// ---------------------------------------------------------------------------
// Addresses
// ---------------------------------------------------------------------------
constexpr uintptr_t ADDR_FrontEndMenuManager = 0xBA6748; // StaticRef<CMenuManager>
constexpr uintptr_t ADDR_aScreens            = 0x8CE008; // tMenuScreen[44]; "FEM_MM" (screen 34) is at 0x8CFE0C in gta_sa.exe
constexpr uintptr_t ADDR_bMissionPackGame    = 0xB72910; // CGame::bMissionPackGame
constexpr uintptr_t ADDR_CText_Get           = 0x6A0050; // CText::Get(const char*), thiscall
constexpr uintptr_t ADDR_MenuCodeBegin       = 0x572EC0; // CMenuManager code range
constexpr uintptr_t ADDR_MenuCodeEnd         = 0x580000;
constexpr uintptr_t ADDR_MenuDraw            = 0x57C290; // CMenuManager::DrawFrontEnd
constexpr uintptr_t ADDR_MenuDrawCalls[]     = { 0x53E82D, 0x53EB8C }; // its two call sites (before / during game session)
constexpr uintptr_t ADDR_MenuValueTextCall   = 0x57A161; // DrawStandardMenu: CText::Get for the value column of a language row
constexpr uintptr_t ADDR_PcOptionsCall       = 0x576FEF; // call to CMenuManager::ProcessPCMenuOptions inside ProcessMenuOptions
constexpr uintptr_t ADDR_VersionProbe       = 0x401000; // GAME_10US_COMPACT: 0x53EC8B55, HOODLUM: 0x16197BE9

inline MenuManager& Menu()            { return *reinterpret_cast<MenuManager*>(ADDR_FrontEndMenuManager); }
inline MenuScreen&  Screen(int idx)   { return reinterpret_cast<MenuScreen*>(ADDR_aScreens)[idx]; }
inline uint8_t&     MissionPackGame() { return *reinterpret_cast<uint8_t*>(ADDR_bMissionPackGame); }

} // namespace game
